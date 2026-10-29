#pragma once

#include <cstdint>
#include <string_view>
#include <filesystem>

#include "../node.hpp"
#include "../lsp_process.hpp"

struct code_node : node {
    inline static uint32_t t_id;

    enum token_modifiers : uint32_t {
        DEFAULT_LIB = 1
    };

    enum token_type : uint8_t {
        NEWL, END, NONE, KEYWORD, TYPE, NUMBER, BOOLEAN, STRING, OPERATOR,
        FUNCTION, COMMENT, MACRO, PARAMETER, CLASS, PROPERTY, ENUM,
        ENUM_MEMBER
    };

    struct token_t {
        std::string_view str;
        token_type type = NONE;
        uint32_t color;
        bool italic = false;

        token_t(std::string_view str, token_type type, uint32_t color, bool italic = false) : str(str), type(type), color(color), italic(italic) {}
    };

    std::string content;
    std::vector<token_t> tokens{};
    std::string lang;

    std::filesystem::path src;

    code_node(std::string lang) : lang(lang) {}
    
    code_node(std::string lang, std::filesystem::path src) : lang(lang), src(src) {
        if (!src.empty()) content = modoc::read_file_as_string(src.string());  
    }

    virtual uint32_t type_id() const override {
        return code_node::t_id;
    }


    const char* type() const override {
        return "code";
    }

    uint8_t scope_end() override {
        return scope_end::ENDSCP;
    }


    void parse_tokens(std::vector<modoc::string_type>&& new_tokens, uint8_t tabs) override {}

    struct theme {
        uint32_t background; // Base
        uint32_t lineNumbers; // Overlay 1
        uint32_t text; // Text
        uint32_t keywords; // Mauve
        uint32_t types; // Yellow
        uint32_t operators; // Sky
        uint32_t numbers; // Peach
        uint32_t strings; // Green
        uint32_t functions; // Blue
        uint32_t macros; // Mauve
        uint32_t comments; // Overlay 2
        uint32_t parameters; // Maroon
        uint32_t classes; // Yellow
        uint32_t properties; // Lavender
        uint32_t enums; // Yellow
        uint32_t enum_members; // Teal
        uint32_t booleans; // Teal

        uint32_t default_lib; // Peach
    } _theme;

    static uint32_t argb(uint8_t type, const theme& t) { // TODO: remove this
        switch (type) {
            case KEYWORD: return t.keywords; 
            case TYPE: return t.types;
            case OPERATOR: return t.operators;
            case NUMBER: return t.numbers;
            case STRING: return t.strings;
            case FUNCTION: return t.functions;
            case COMMENT: return t.comments;
            case BOOLEAN: return t.booleans;
        }
        return t.text;
    }

    //TODO: Could be combined with lsp modoc tokenize????
    static size_t tokenize_value(std::vector<token_t>& tokens, std::string_view str, const theme& theme) {
        static const std::unordered_set<std::string_view> types {"char", "short", "int", "long", "float", "double", "void", "bool"};
        static const std::unordered_set<std::string_view> keywords {"if", "else", "return", "throw", "for", "constexpr", "struct", "class", "template", "typename"};

        const char* ptr = str.data();
        uint8_t type = NONE;
        uint32_t len = 0;

        if (*ptr == '"') {
            const char* const begin = ptr;
            bool slash = false;

            ++ptr;
            while (ptr < str.end() && (*ptr != '"' || slash)) {
                if (!slash && *ptr == '\\') slash = true;
                else slash = false;
                ++ptr;
            }
            if (ptr < str.end() && *ptr == '"') ++ptr;
            
            type = STRING;
            len = ptr - begin;
        }
        else if (ptr + 1 < str.end() && ptr[0] == '/' && ptr[1] == '/') {
            const char* const begin = ptr;
            ptr += 2;

            while (ptr < str.end() && *ptr != '\n') ++ptr;

            len = ptr - begin;

            tokens.emplace_back(std::string_view{str.data(), str.data() + len}, COMMENT, theme.comments, true);
            return len;
        }
        else if (ptr + 1 < str.end() && ptr[0] == '/' && ptr[1] == '*') {
            const char* begin = ptr;
            ptr += 2;

            while (ptr < str.end()) {
                if (*ptr == '\n') {
                    tokens.emplace_back(std::string_view{begin, ptr}, COMMENT, theme.comments, true);
                    tokens.emplace_back(std::string_view{ptr, ptr + 1}, NEWL, 0);
                    begin = ptr + 1; 
                }
                else if (ptr + 1 < str.end() && ptr[0] == '*' && ptr[1] == '/') {
                    ptr += 2;
                    break;
                }
                ++ptr;
            }

            if (begin < ptr) tokens.emplace_back(std::string_view{begin, ptr}, COMMENT, theme.comments, true);

            return ptr - str.data();
        }
        else if (modoc::operator_chars.contains(*ptr)) {
            type = NONE;
            len = 1;
            while (modoc::operator_chars.contains(*++ptr)) ++len;
        }
        else {
            double num;
            auto [read_end, ec] = std::from_chars(ptr, str.end(), num);

            if (ptr != read_end && ec == std::errc{}) {
                type = NUMBER;
                len = read_end - ptr;
            }
            else {
                const char* begin = ptr;
                while (ptr < str.end() && *ptr > ' ' && !modoc::operator_chars.contains(*ptr)) ++ptr;

                len = ptr - begin;

                if ((len == 4 && strncmp(begin, "true", 4) == 0) || (len == 5 && strncmp(begin, "false", 5) == 0)) type = BOOLEAN;
                else if (types.contains({begin, ptr})) type = TYPE;
                else if (keywords.contains({begin, ptr})) type = KEYWORD;
                else if (len == 6 && strncmp(begin, "sizeof", 6) == 0) type = OPERATOR;
                else if (len == 8 && strncmp(begin, "#include", 8) == 0) {
                    tokens.emplace_back(std::string_view{begin, ptr}, KEYWORD, theme.keywords);

                    do {
                        ++ptr;
                    } while (ptr < str.end() && *ptr != '<');

                    begin = ptr;

                    do {
                        ++ptr;
                    } while (ptr < str.end() && *ptr != '>');
                    ++ptr;

                    tokens.emplace_back(std::string_view{begin, ptr}, STRING, theme.strings);
                    return ptr - str.data();
                }
            }
        }

        tokens.emplace_back(std::string_view{str.data(), str.data() + len}, (token_type)type, argb(type, theme));
        return len;
    }

    static token_t get_token(std::string_view type, std::string_view token_str, const theme& theme) {
        if (type == "keyword") return {token_str, KEYWORD, theme.keywords};
        else if (type == "operator") return {token_str, OPERATOR, theme.operators};
        else if (type == "function") return {token_str, FUNCTION, theme.functions};
        else if (type == "type") return {token_str, TYPE, theme.types};
        else if (type == "comment") return {token_str, COMMENT, theme.comments};
        else if (type == "macro") return {token_str, MACRO, theme.macros};
        else if (type == "parameter") return {token_str, PARAMETER, theme.parameters};
        else if (type == "class") return {token_str, CLASS, theme.classes};
        else if (type == "property") return {token_str, PROPERTY, theme.properties};
        else if (type == "enum") return {token_str, ENUM, theme.enums};
        else if (type == "enumMember") return {token_str, ENUM_MEMBER, theme.enum_members};

        return {token_str, NONE, theme.text};
    }

    static std::vector<token_t> tokenize(std::string_view str, const theme& theme, std::string_view src) {
        std::vector<token_t> result;
        const char* begin = nullptr;

        for (const char* ptr = str.data(); ptr < str.end(); ++ptr) {
            /*if (begin == nullptr && *ptr > ' ') begin = ptr;
            else if (begin != nullptr && *ptr <= ' ') {
                const std::string_view view = {begin, ptr};

                if (types.contains(view)) result.emplace_back(view, TYPE);
                else if (keywords.contains(view)) result.emplace_back(view, KEYWORD);
                else result.emplace_back(view, NONE);

                begin = nullptr;
            }*/
            if (*ptr > ' ' /*&& !modoc::operator_chars.contains(*ptr)*/) {
                ptr += tokenize_value(result, {ptr, str.end()}, theme) - 1;
                std::cout << result.back().str << '\n';
            }
            else if (*ptr == '\n') result.emplace_back(std::string_view{ptr, ptr + 1}, NEWL, 0);
        }

        if (begin != nullptr) {
            const std::string_view view = {begin, str.end()};
            tokenize_value(result, view, theme);
            
            /*if (types.contains(view)) result.emplace_back(view, TYPE);
            else if (keywords.contains(view)) result.emplace_back(view, KEYWORD);
            else result.emplace_back(view, NONE);*/
        }

        {
            modoc::lsp_process lsp;
            const std::string name = src.empty() ? "file:///tmp/main.cpp" : (std::string("file://") + std::string(src));

            lsp.init("clangd");

            lsp.open_document({name, "cpp", str, 1});
            std::vector<modoc::lsp_process::token> tokens = lsp.request_tokens(name);
            lsp.close_document(name);

            lsp.shutdown();

            auto itr1 = result.begin();
            auto itr2 = tokens.begin();

            while (itr1 < result.end() && itr2 < tokens.end()) {
                if (itr2->str.begin() < itr1->str.begin()) {
                    std::cout << '<' << itr1->str << ' ' << itr2->str << '\n';
                    itr1 = result.insert(itr1, get_token(lsp.token_types[itr2->type_id], itr2->str, theme));

                    ++itr1;

                    while (itr1 < result.end() && itr1->str.begin() < itr2->str.end())
                        itr1 = result.erase(itr1);

                    ++itr2;
                }
                else if (itr2->str.begin() == itr1->str.begin()) {
                    std::cout << '=' << itr1->str << ' ' << itr2->str << '\n';
                    *itr1 = get_token(lsp.token_types[itr2->type_id], itr2->str, theme);

                    ++itr1;

                    while (itr1 < result.end() && itr1->str.begin() < itr2->str.end())
                        itr1 = result.erase(itr1);

                    ++itr2;
                }
                else ++itr1;
            }

            while (itr2 < tokens.end()) {
                result.push_back(get_token(lsp.token_types[itr2->type_id], itr2->str, theme));
                ++itr2;
            }
        }

        return result;
    }

    theme get_theme() const {
        const value* ptr = get_meta("theme");
        if (ptr == nullptr) ptr = parent->get_variable("theme");

        theme t;

        if (ptr != nullptr && ptr->type() == value::OBJECT) {
            const value::object_t& obj = ptr->object();

            t.background = obj.at("background").number();
            t.numbers = obj.at("color7").number();
            t.keywords = obj.at("color4").number();
            t.functions = obj.at("color13").number();
            t.operators = obj.at("color11").number();
            t.lineNumbers = obj.at("text4").number();
            t.strings = obj.at("color9").number();
            t.types = obj.at("color8").number();
            t.text = obj.at("text").number();
            t.macros = t.keywords;
            t.comments = obj.at("text5").number();
            t.parameters = obj.at("color6").number();
            t.classes = t.types;
            t.properties = obj.at("color14").number();
            t.enums = t.types;
            t.enum_members = obj.at("color10").number();
            t.booleans = t.numbers;

            t.default_lib = t.numbers;
        }

        return t;
    }
    
    void parse_verbatim(std::string_view str, bool to_copy) override {
        if (content.empty()) {
            modoc::logger::s_log("code", "verbatim", str);
            content = str;
        }
    }

    virtual const value* get_meta(std::string_view name) const override {
        const value* ptr = node::get_meta(name);
        if (ptr != nullptr) return ptr;

        ptr = parent->get_variable("theme");

        if (ptr != nullptr && ptr->type() == value::OBJECT) {
            const value::object_t& obj = ptr->object();

            if (name == "background" && obj.contains_type("background1", value::NUMBER)) return &obj.at("background1");
            else if (name == "lineNumbers" && obj.contains_type("surface1", value::NUMBER)) return &obj.at("surface1");
        }

        return nullptr;
    }

    /*virtual void debug_print() const override {
        printf("[code](lang = %s)\n", lang.c_str());
    }*/

    void finalize() override {
        if (!meta.contains("padding")) meta["padding"] = 6;
        
        _theme = get_theme();
        tokens = tokenize(content, _theme, src.string());
            
        std::string result;
        for (auto& t : tokens) {
            result += '(';
            result += t.str;
            result += ", {";
            result += std::string_view{t.str.end(), t.str.end() + 3};
            result += "}, ";
            result += std::to_string(t.type);
            result += ')';
        }
        modoc::logger::s_log("code", "finalize", result);
    }

    const std::vector<node*>* child_nodes() const override {
        return nullptr;
    }

    modoc::tree* subtree() override {
        return nullptr;
    }

    bool verbatim() const override {
        return true;
    }

    void add_node(node*) override {}

    ~code_node() override {}

    struct content_t {
        const std::string_view str;
        const std::span<const token_t> tokens;
        
        const uint32_t text_color;

        struct content_iterator {
            const char* pos;
            const char* const end;

            const token_t* token_pos;
            const token_t* const token_end;

            const uint32_t text_color;

            content_iterator(std::string_view str, std::span<const token_t> tokens, uint32_t text_color) : 
                pos(str.data()), end(str.end()), token_pos(tokens.data()), token_end(tokens.end().base()),
                text_color(text_color) {}

            content_iterator& operator++() {
                if (token_pos < token_end) {
                    if (pos == token_pos->str.begin()) {
                        pos = token_pos->str.end();
                        ++token_pos;
                    }
                    else { // Iterator points to gap
                        pos = token_pos->str.begin(); 
                    }
                }
                else pos = end;

                return *this;
            }

            token_t operator*() const {
                if (token_pos < token_end) {
                    if (pos == token_pos->str.begin()) // Iterator points to token
                        return *token_pos;
                    else
                        return {{pos, token_pos->str.begin()}, NONE, text_color}; 
                }
                else if (pos < end) return {{pos, end}, NONE, text_color};

                return {{}, END, 0};
            }

            bool operator!=(const content_iterator& itr) const {
                return pos != itr.pos; 
            }

            bool operator==(const content_iterator& itr) const {
                return pos == itr.pos; 
            }
        };

        content_t(std::string_view str, std::span<const token_t> tokens, uint32_t text_color) :
            str(str), tokens(tokens), text_color(text_color) {}

        content_iterator begin() const {
            return {str, tokens, text_color};
        }

        content_iterator end() const {
            return {{str.end(), str.end()}, {tokens.end(), tokens.end()}, text_color};
        }
    };

    content_t _content() const {
        return content_t(content, tokens, 0);
    }
};

struct code_f : node_factory {

    void init() override {
        value obj = value::from_object({});
        value::object_t& map = obj.object();

        {
            map["lang"] = value::from_object({
                {"cpp", value("cpp")}
            }); 
        }

        register_constant("code", std::move(obj));
    }

    node* instance(modoc::tree& parent, uint8_t, const options_t& op) override {
        std::filesystem::path src;
        if (op.contains("src") && op.at("src").type() == value::STRING) {
            src = modoc::paths.document_resource(op.at("src").string());    
        }

        if (op.contains("lang") && op.at("lang").type() == value::STRING) {
            return new code_node((std::string)op.at("lang").string(), src);
        }
        return nullptr;
    }

    void set_node_type_id(uint32_t id) const override {
        code_node::t_id = id;
    }
};
