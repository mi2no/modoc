// Simple, dependency-free PDF backend.
//
// Mirrors the node coverage of htmlNG2.cpp (group, sec, list, code, text) but,
// unlike HTML, PDF has no reflow engine of its own: this backend has to do its
// own word-wrapping and page-breaking. It writes raw PDF bytes directly
// (objects + xref + trailer) using the standard base-14 fonts (Times-Roman,
// Times-Bold, Courier), so no font embedding / external libs are needed.
// Times is used instead of Helvetica to read closer to LaTeX's default serif
// look (Computer Modern itself isn't a base-14 font, so isn't available
// without embedding).
//
// Known limitation: the base-14 fonts only cover WinAnsiEncoding (~Latin-1).
// Polish diacritics (ą ć ę ł ń ś ź ż) aren't in that set, so they're
// transliterated to their plain-ASCII base letter. Real glyph support would
// require embedding a TrueType font (Identity-H / subsetting), which is out
// of scope for this provisional backend.

#include <algorithm>
#include <cstdio>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>
#include <array>

#include "../node.hpp"
#include "../nodes/code_temp.hpp"

namespace pdfback {

// ---------------------------------------------------------------------------
// Text encoding: UTF-8 -> single-byte WinAnsi-ish encoding usable by the
// base-14 fonts, with a small transliteration table for Polish letters.
// ---------------------------------------------------------------------------

static uint32_t utf8_decode(std::string_view s, size_t& i) {
    unsigned char c = s[i];

    if (c < 0x80) { ++i; return c; }

    int extra;
    uint32_t cp;

    if ((c & 0xE0) == 0xC0) { extra = 1; cp = c & 0x1F; }
    else if ((c & 0xF0) == 0xE0) { extra = 2; cp = c & 0x0F; }
    else if ((c & 0xF8) == 0xF0) { extra = 3; cp = c & 0x07; }
    else { ++i; return '?'; }

    if (i + extra >= s.size()) { ++i; return '?'; }

    for (int k = 1; k <= extra; ++k) {
        unsigned char cc = s[i + k];
        if ((cc & 0xC0) != 0x80) { i += 1; return '?'; }
        cp = (cp << 6) | (cc & 0x3F);
    }

    i += extra + 1;
    return cp;
}

static unsigned char unicode_to_pdf_byte(uint32_t cp) {
    if (cp < 0x80) return (unsigned char)cp;
    if (cp >= 0xA0 && cp <= 0xFF) return (unsigned char)cp; // Latin-1 == WinAnsi here

    switch (cp) {
        // Polish letters without a WinAnsi glyph: transliterate to base ASCII.
        case 0x0104: return 'A'; case 0x0105: return 'a'; // A/a ogonek
        case 0x0106: return 'C'; case 0x0107: return 'c'; // C acute
        case 0x0118: return 'E'; case 0x0119: return 'e'; // E ogonek
        case 0x0141: return 'L'; case 0x0142: return 'l'; // L stroke
        case 0x0143: return 'N'; case 0x0144: return 'n'; // N acute
        case 0x015A: return 'S'; case 0x015B: return 's'; // S acute
        case 0x0179: return 'Z'; case 0x017A: return 'z'; // Z acute
        case 0x017B: return 'Z'; case 0x017C: return 'z'; // Z dot
        default: return '?';
    }
}

// Re-encodes a UTF-8 string into the single-byte string this backend renders.
static std::string to_pdf_text(std::string_view utf8) {
    std::string out;
    out.reserve(utf8.size());

    size_t i = 0;
    while (i < utf8.size())
        out += (char)unicode_to_pdf_byte(utf8_decode(utf8, i));

    return out;
}

static std::string escape_pdf_string(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 4);

    for (char c : s) {
        if (c == '(' || c == ')' || c == '\\') out += '\\';
        out += c;
    }

    return out;
}

// ---------------------------------------------------------------------------
// Font metrics (AFM widths, per-1000-em), just enough to word-wrap.
// ---------------------------------------------------------------------------

// Times-Roman widths for ASCII 32..126 (also used as an approximation for
// Times-Bold, which is close enough for the short heading lines it's used
// on). Times is the closest base-14 stand-in for LaTeX's serif look
// (Computer Modern itself isn't one of the 14 standard fonts).
static const std::array<uint16_t, 95> serif_widths = {
    250,333,408,500,500,833,778,180,333,333,500,564,250,333,250,278, // ' ' .. '/'
    500,500,500,500,500,500,500,500,500,500,278,278,564,564,564,444, // '0' .. '?'
    921,722,667,667,722,611,556,722,722,333,389,722,611,889,722,722, // '@' .. 'O'
    556,722,667,556,611,722,722,944,722,722,611,333,278,333,469,500, // 'P' .. '_'
    333,444,500,444,500,444,333,500,500,278,278,500,278,778,500,500, // '`' .. 'o'
    500,500,333,389,278,500,500,722,500,500,444,480,200,480,541 // 'p' .. '~'
};

static double char_width(unsigned char c, bool monospace) {
    if (c == '\t') return 2400.0;
    if (monospace) return 600.0; // Courier is fixed-pitch

    if (c >= 32 && c <= 126) return serif_widths[c - 32];
    return 500.0; // fallback for anything outside the ASCII table
}

static double text_width(std::string_view s, double size, bool monospace) {
    double w = 0;
    for (unsigned char c : s) w += char_width(c, monospace);
    return w / 1000.0 * size;
}

// Greedy word-wrap. Splits only on spaces (already-encoded text).
static std::vector<std::string> wrap_text(std::string_view s, double size, double max_width, bool monospace) {
    std::vector<std::string> lines;
    std::string current;

    size_t pos = 0;
    while (pos <= s.size()) {
        size_t next = s.find(' ', pos);
        std::string_view word = (next == std::string_view::npos) ? s.substr(pos) : s.substr(pos, next - pos);

        if (!word.empty()) {
            std::string candidate = current.empty() ? std::string(word) : current + ' ' + std::string(word);

            if (!current.empty() && text_width(candidate, size, monospace) > max_width) {
                lines.push_back(current);
                current = word;
            }
            else current = candidate;
        }

        if (next == std::string_view::npos) break;
        pos = next + 1;
    }

    if (!current.empty() || lines.empty()) lines.push_back(current);
    return lines;
}

// ---------------------------------------------------------------------------
// Low level PDF file assembly: indirect objects + xref + trailer.
// ---------------------------------------------------------------------------

struct rgb { double r, g, b; };

struct pdf_writer {
    static constexpr double PAGE_W = 595.0, PAGE_H = 842.0, MARGIN = 50.0;
    static constexpr double CONTENT_W = PAGE_W - 2 * MARGIN;

    std::vector<std::string> objects; // objects[id - 1] == body of object `id`
    std::vector<int> page_ids;

    int font_serif, font_serif_bold, font_courier;

    std::string page_stream;
    double cursor_y;

    pdf_writer() {
        font_serif = add_font("Times-Roman");
        font_serif_bold = add_font("Times-Bold");
        font_courier = add_font("Courier");

        start_page();
    }

    int reserve() {
        objects.emplace_back();
        return (int)objects.size();
    }

    int add_object(std::string body) {
        objects.push_back(std::move(body));
        return (int)objects.size();
    }

    int add_font(const char* base_name) {
        std::string body = "<< /Type /Font /Subtype /Type1 /BaseFont /";
        body += base_name;
        body += " /Encoding /WinAnsiEncoding >>";
        return add_object(body);
    }

    void start_page() {
        page_stream.clear();
        cursor_y = PAGE_H - MARGIN;
    }

    void end_page() {
        int content_id = add_object(
            "<< /Length " + std::to_string(page_stream.size()) + " >>\nstream\n" +
            page_stream + "\nendstream");

        std::string page_body = "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 " +
            std::to_string((int)PAGE_W) + ' ' + std::to_string((int)PAGE_H) + "]"
            " /Resources << /Font << /FSerif " + std::to_string(font_serif) + " 0 R"
            " /FSerifB " + std::to_string(font_serif_bold) + " 0 R"
            " /FCourier " + std::to_string(font_courier) + " 0 R >> >>"
            " /Contents " + std::to_string(content_id) + " 0 R >>";

        page_ids.push_back(add_object(page_body));
    }

    void new_page() {
        end_page();
        start_page();
    }

    // Makes sure `needed` vertical space remains above the bottom margin,
    // starting a new page first if it doesn't.
    void ensure_space(double needed) {
        if (cursor_y - needed < MARGIN) new_page();
    }

    void draw_rect(double x, double y, double w, double h, rgb color) {
        char buf[160];
        snprintf(buf, sizeof(buf), "q %.3f %.3f %.3f rg %.2f %.2f %.2f %.2f re f Q\n",
            color.r, color.g, color.b, x, y, w, h);
        page_stream += buf;
    }

    // Same as draw_rect, but with corners rounded to `radius`. Builds the
    // outline as four straight edges joined by cubic-Bezier quarter-circle
    // arcs (the standard kappa ~= 0.5523 circle approximation), then fills
    // it as one closed path.
    void draw_rounded_rect(double x, double y, double w, double h, double radius, rgb color) {
        double r = std::min({radius, w / 2.0, h / 2.0});
        if (r <= 0.0) { draw_rect(x, y, w, h, color); return; }

        constexpr double k = 0.5522847498; // 4/3 * (sqrt(2) - 1)
        double o = r * k;

        char buf[768];
        snprintf(buf, sizeof(buf),
            "q %.3f %.3f %.3f rg\n"
            "%.2f %.2f m\n"
            "%.2f %.2f l\n"
            "%.2f %.2f %.2f %.2f %.2f %.2f c\n"
            "%.2f %.2f l\n"
            "%.2f %.2f %.2f %.2f %.2f %.2f c\n"
            "%.2f %.2f l\n"
            "%.2f %.2f %.2f %.2f %.2f %.2f c\n"
            "%.2f %.2f l\n"
            "%.2f %.2f %.2f %.2f %.2f %.2f c\n"
            "h f Q\n",
            color.r, color.g, color.b,
            x + r, y,                                                          // start, bottom edge left end
            x + w - r, y,                                                      // bottom edge right end
            x + w - r + o, y, x + w, y + r - o, x + w, y + r,                   // bottom-right arc
            x + w, y + h - r,                                                  // right edge top end
            x + w, y + h - r + o, x + w - r + o, y + h, x + w - r, y + h,       // top-right arc
            x + r, y + h,                                                      // top edge left end
            x + r - o, y + h, x, y + h - r + o, x, y + h - r,                   // top-left arc
            x, y + r,                                                          // left edge bottom end
            x, y + r - o, x + r - o, y, x + r, y);                             // bottom-left arc
        page_stream += buf;
    }

    void draw_text(const char* font_res, double size, double x, double y,
                    std::string_view text, rgb color, bool italic = false) {
        if (text.empty()) return;

        char buf[256];
        std::string esc = escape_pdf_string(text);

        double shear = italic ? 0.25 : 0.0;
        snprintf(buf, sizeof(buf),
            "q %.3f %.3f %.3f rg BT /%s %.2f Tf 1 0 %.3f 1 %.2f %.2f Tm (",
            color.r, color.g, color.b, font_res, size, shear, x, y);

        page_stream += buf;
        page_stream += esc;
        page_stream += ") Tj ET Q\n";
    }

    // Writes a paragraph, wrapping it to fit `width` starting at x_indent,
    // paging as needed. Returns the total height consumed.
    double write_paragraph(std::string_view text, const char* font_res, double size,
                            double line_height, double x_indent, rgb color,
                            bool monospace, bool italic = false) {
        std::vector<std::string> lines = wrap_text(text, size, CONTENT_W - x_indent, monospace);

        for (const std::string& line : lines) {
            ensure_space(line_height);
            cursor_y -= line_height;
            draw_text(font_res, size, MARGIN + x_indent, cursor_y, line, color, italic);
        }

        return lines.size() * line_height;
    }

    std::string finish() {
        end_page();

        int pages_id = reserve(); // object 2, filled in below
        std::string kids = "[";
        for (int id : page_ids) { kids += std::to_string(id); kids += " 0 R "; }
        kids += ']';
        objects[pages_id - 1] = "<< /Type /Pages /Kids " + kids +
            " /Count " + std::to_string(page_ids.size()) + " >>";

        int catalog_id = add_object("<< /Type /Catalog /Pages " + std::to_string(pages_id) + " 0 R >>");

        std::string out = "%PDF-1.4\n";
        std::vector<size_t> offsets(objects.size() + 1, 0);

        for (size_t i = 0; i < objects.size(); ++i) {
            offsets[i + 1] = out.size();
            out += std::to_string(i + 1) + " 0 obj\n" + objects[i] + "\nendobj\n";
        }

        size_t xref_offset = out.size();
        out += "xref\n0 " + std::to_string(objects.size() + 1) + "\n";
        out += "0000000000 65535 f \n";

        char line[32];
        for (size_t i = 1; i <= objects.size(); ++i) {
            snprintf(line, sizeof(line), "%010zu 00000 n \n", offsets[i]);
            out += line;
        }

        out += "trailer\n<< /Size " + std::to_string(objects.size() + 1) +
            " /Root " + std::to_string(catalog_id) + " 0 R >>\n";
        out += "startxref\n" + std::to_string(xref_offset) + "\n%%EOF";

        return out;
    }
};

// ---------------------------------------------------------------------------
// Node -> layout. Structural analogue of htmlNG2.cpp's node_to_str, but
// stateful: every node writes directly into the PDF page stream(s) instead
// of building an intermediate string, since layout needs running knowledge
// of the current page and vertical cursor.
// ---------------------------------------------------------------------------

static const rgb COLOR_TEXT   {0.10, 0.10, 0.10};
static const rgb COLOR_HEADER {0.15, 0.15, 0.15};
static const rgb COLOR_BULLET {0.35, 0.35, 0.35};
static const rgb COLOR_CODE_BG{0.93, 0.93, 0.93};
static const rgb COLOR_TYPE   {0.55, 0.42, 0.05};
static const rgb COLOR_KEYWORD{0.45, 0.20, 0.60};

constexpr double BODY_SIZE = 11.0;
constexpr double BODY_LINE = 15.0;

static void node_to_pdf(const node* n, pdf_writer& doc, double indent);

static void children_to_pdf(const node* n, pdf_writer& doc, double indent) {
    const std::vector<node*>* children = n->child_nodes();
    if (children == nullptr) return;
    for (const node* ch : *children) node_to_pdf(ch, doc, indent);
}

static void write_sec(const sec_node* s, pdf_writer& doc, double indent) {
    std::string number = std::to_string(s->id[0]);
    for (uint8_t i = 1; i <= s->depth; ++i) {
        number += '.';
        number += std::to_string(s->id[i]);
    }

    double size = std::max(20.0 - s->depth * 2.0, 13.0);
    double line_h = size + 6.0;

    doc.ensure_space(line_h + 10.0);
    doc.cursor_y -= 10.0; // spacing before heading

    std::string heading = number + "  " + s->title;
    doc.write_paragraph(to_pdf_text(heading), "FSerifB", size, line_h, indent, COLOR_HEADER, false);

    doc.cursor_y -= 4.0;

    children_to_pdf(s, doc, indent + 12.0);
}

static void write_list(const list_node* l, pdf_writer& doc, double indent) {
    const std::vector<node*>* children = l->child_nodes();
    if (children == nullptr) return;

    for (const node* ch : *children) {
        doc.ensure_space(BODY_LINE);
        // Bullet, drawn at the current line's baseline.
        doc.draw_text("FSerif", BODY_SIZE, doc.MARGIN + indent, doc.cursor_y - BODY_LINE + 3.0,
                       "-", COLOR_BULLET, false);
        node_to_pdf(ch, doc, indent + 14.0);
    }
}

static void write_code(const code_node* c, pdf_writer& doc, double indent) {
    constexpr double size = 9.5, line_h = 13.0, pad = 8.0, tab_w = 4 * 600.0 / 1000.0 * size;

    // Pre-flatten tokens into lines so the background box height is known
    // up front (and so a line never gets split across a page break oddly).
    struct piece { std::string_view text; code_node::token_type type; };
    std::vector<std::vector<piece>> lines;
    uint8_t tabs = 0;

    /*for (const code_node::token_t& t : c->tokens) {
        if (t.type == code_node::NEWL) {
            tabs = (uint8_t)t.str[0];
            lines.push_back({});
        }
        else {
            if (lines.empty()) lines.push_back({});
            lines.back().push_back({t.str, t.type});
        }
    }*/


    const code_node::token_t* t = c->tokens.data();
    const char* ptr = c->content.data();
    while (t < c->tokens.end().base()) {
        if (ptr == t->str.begin()) {
            if (t->type == code_node::NEWL) lines.push_back({});
            else {
                if (lines.empty()) lines.push_back({});
                lines.back().push_back({t->str, t->type});
            }
            
            ptr = t->str.end();
            ++t;
        }
        else {
            if (lines.empty()) lines.push_back({});
            lines.back().emplace_back(std::string_view{ptr, t->str.data()}, code_node::NONE);
            ptr = t->str.begin();
        }
    }

    doc.ensure_space(line_h * 2 + pad * 2);
    doc.cursor_y -= 6.0;

    std::string lang_label = "[" + c->lang + "]";
    doc.write_paragraph(lang_label, "FCourier", size, line_h, indent, COLOR_BULLET, true);

    doc.ensure_space(line_h * lines.size());

    // One rect for the whole block: anchor on cursor_y as it stands right
    // now (before any of the code lines are drawn) and size it to the full
    // block height, rather than reusing the single-line offset per line.
    const double block_top = doc.cursor_y;
    const double block_h = line_h * lines.size();
    doc.draw_rounded_rect(doc.MARGIN + indent - 4.0, block_top - block_h + 3.0,
                           doc.CONTENT_W - indent + 4.0, block_h, 5.0, COLOR_CODE_BG);

    for (const std::vector<piece>& line : lines) {
        double x = doc.MARGIN + indent + tabs * tab_w;
        for (const piece& p : line) {
            rgb color = COLOR_TEXT;
            bool italic = false;

            switch (p.type) {
                case code_node::TYPE: color = COLOR_TYPE; break;
                case code_node::KEYWORD: color = COLOR_KEYWORD; italic = true; break;
                default: break;//color = {.9, .9, .9};
            }

            std::string text = to_pdf_text(p.text);
            doc.draw_text("FCourier", size, x, doc.cursor_y - line_h + 3.0 + (line_h - size) * 0.3,
                          text, color, italic);
            x += text_width(text, size, true);// + text_width(" ", size, true);
        }

        doc.cursor_y -= line_h;
    }

    doc.cursor_y -= 6.0;
}

static void write_text(const text_node* t, pdf_writer& doc, double indent) {
    std::string joined;
    for (const modoc::string_type& s : t->tokens) {
        joined += to_pdf_text(s.view());
        joined += ' ';
    }
    if (!joined.empty()) joined.pop_back();

    doc.write_paragraph(joined, "FSerif", BODY_SIZE, BODY_LINE, indent, COLOR_TEXT, false);
    doc.cursor_y -= 4.0; // paragraph spacing
}

static void node_to_pdf(const node* n, pdf_writer& doc, double indent) {
    if (node::is_type<sec_node>(n)) write_sec((const sec_node*)n, doc, indent);
    else if (node::is_type<list_node>(n)) write_list((const list_node*)n, doc, indent);
    else if (node::is_type<group_node>(n)) children_to_pdf(n, doc, indent);
    else if (node::is_type<code_node>(n)) write_code((const code_node*)n, doc, indent);
    else if (node::is_type<text_node>(n)) write_text((const text_node*)n, doc, indent);
    // Anything else (plugin-defined nodes) is expected to already have been
    // lowered to one of the above by the time it reaches a backend.
}

} // namespace pdfback

extern "C" void compile(const std::vector<node*>& tree) {
    pdfback::pdf_writer doc;

    for (const node* n : tree)
        pdfback::node_to_pdf(n, doc, 0.0);

    std::string result = doc.finish();

    FILE* const out = fopen("out.pdf", "wb");
    fwrite(result.data(), 1, result.size(), out);
    fclose(out);
}
