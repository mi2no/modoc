#include <cstdint>
#include <cstdio>
#include <format>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "../node.hpp"
#include "../log.hpp"

#include "../nodes/code_temp.hpp"

const char* encode_char(char c) {
    switch (c) {
        case '<': return "&lt;";
        case '>': return "&gt;";
    }
    return nullptr;
}

void add_formated(std::string& str, std::string_view addition) {
    const char* begin = addition.data();
    for (const char* ptr = begin; ptr < addition.end(); ++ptr) {
        const char* const encoded = encode_char(*ptr);

        if (encoded != nullptr) {
            str += {begin, ptr};
            str += encoded;
            begin = ptr + 1;
        }
    }
    if (begin < addition.end()) str += {begin, addition.end()};
}

std::string node_to_str(const node* n) {
    std::string result, style;

    puts(n->type());
    fflush(stdout);

    if (n->meta.size()) {
        style = "style=\"";
        for (const auto& entry : n->meta) {
            style += entry.first;
            style += ": ";
            style += entry.second.to_string();
            style += ';';
        }
        style += '"';
    }

    if (node::is_type<group_node>(n)) {
        const group_node* g = (const group_node*)n;
        
        result += "<div "; 
        if (style.size()) result += style;
        result += '>';

        const std::vector<node*>* children = n->child_nodes();

        if (children != nullptr)
            for (const node* ch : *children)
                result += node_to_str(ch);

        result += "</div>";
    }
    else if (node::is_type<sec_node>(n)) {
        const sec_node* s = (const sec_node*)n;
        
        result += "<section>\n<h2>";
        result += s->title;
        result += "</h2>\n";

        const std::vector<node*>* children = n->child_nodes();

        if (children != nullptr)
            for (const node* ch : *children)
                result += node_to_str(ch);

        result += "</section>\n";
    }
    else if (node::is_type<list_node>(n)) {
        list_node* l = (list_node*)n;
        //result += "<ul>\n";
        result += "<div class=\"list\"";
        if (style.size()) result += style;
        result += ">\n";
 
        const std::vector<node*>* children = n->child_nodes();

        if (children != nullptr)
            for (const node* ch : *children) {
                //result += "<li>\n";
                result += "<div>\n";
                result += node_to_str(ch);
                result += "</div>\n";
                //result += "</li>\n";
            }

        //result += "</ul>\n";
        result += "</div>\n";
    }
    else if (node::is_type<code_node>(n)) {
        code_node* c = (code_node*)n;

        std::vector<std::vector<code_node::token_t>> lines;
        uint8_t tabs = 0;

        const uint32_t color_background = c->get_meta("background")->number();//c->meta.at("theme").object().at("background").number());
        //const uint32_t color_line_nums = c->get_meta("lineNumbers")->number();//c->meta.at("theme").object().at("lineNumbers").number());

        //const double padding = c->meta.at("padding").number();

        const code_node::token_t* t = c->tokens.data();
        const char* ptr = c->content.data();
        while (t < c->tokens.end().base()) {
            if (ptr == t->str.begin()) {
                if (t->type == code_node::NEWL) lines.push_back({});
                else {
                    if (lines.empty()) lines.push_back({});
                    lines.back().push_back(*t);
                }
               
                do {
                    ptr = t->str.end();
                    ++t;
                } while (ptr > t->str.data());
            }
            else {
                if (lines.empty()) lines.push_back({});
                lines.back().emplace_back(std::string_view{ptr, t->str.data()}, code_node::NONE, 0xFF000000);
                ptr = t->str.begin();
            }
        }

        result += "<pre style=\"background-color:#";
        result += std::format("{:06x}", color_background & 0x00FFFFFF);
        result += ";\">\n<code>\n";

        uint32_t prev_color = 0;
        for (size_t i = 0; i < lines.size(); ++i) {
            const std::string line_num_str = std::to_string(i + 1);
            /*x += ((int)log10(lines.size()) - (int)log10(i + 1)) * doc.text_width(" ", font, size, true); // right align TODO: remove log10!!!
            doc.draw_text(font, size, x, doc.cursor_y - line_h + 3.0 + (line_h - size) * 0.3, line_num_str, color_line_nums);
            x += doc.text_width(line_num_str, font, size, true);
            x += 10;*/

            for (const code_node::token_t& t : lines[i]) {
                //std::cout << p.text.size() << ' ' << std::string_view{p.text.data(), p.text.data() + 10} << '\n';
                //fflush(stdout);
                if (t.color != 0xFF000000) {
                    result += "<span style=\"color:#";
                    result += std::format("{:06x}", t.color & 0x00FFFFFF);

                    if (t.italic) result += ";font-style:italic;\">";
                    else result += ";\">";

                    add_formated(result, t.str);
                    result += "</span>";
                }
                else result += t.str;
            }
            
            result += '\n';
        }

        result += "</code>\n</pre>\n";
    }
    else if (node::is_type<text_node>(n)) {
        text_node* t = (text_node*)n;

        if (style.size()) {
            result += "<span ";
            result += style;
            result += ">\n";
        }

        for (const modoc::string_type& s : t->tokens) {
            result += s.view();
            result += ' ';
        }

        if (style.size()) result += "\n</span>";

        result += '\n';
    }


    return result;
}

extern "C" void compile(const std::vector<node*>& tree) {
    FILE* const out = fopen("out.html", "w");
    std::string result;

    result += "<!DOCTYPE HTML>\n<html>";

    for (const node* n : tree)
       result += node_to_str(n);

    result += "</html>";

    fputs(result.c_str(), out);

    fclose(out);
}
