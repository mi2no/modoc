#pragma once

#include <cstdint>
#include <string_view>

#include "../node.hpp"
#include "../lsp_process.hpp"

struct code_node : node {
    inline static uint32_t t_id;

    enum token_type : uint8_t {
        NEWL, NONE, KEYWORD, TYPE, NUMBER, BOOLEAN, STRING, OPERATOR, FUNCTION
    };

    struct token_t {
        std::string_view str;
        token_type type = NONE;
        uint32_t color;

        token_t(std::string_view str, token_type type, uint32_t color) : str(str), type(type), color(color) {}
    };

    std::string content;
    std::vector<token_t> tokens{};
    std::string lang;

    code_node(std::string lang) : lang(lang) {}

    virtual uint32_t type_id() const override {
        return code_node::t_id;
    }


    const char* type() const override {
        return "code";
    }

    uint8_t scope_end() override {
        return scope_end::ENDSCP;
    }


    void parse_tokens(std::vector<modoc::string_type>&& new_tokens, uint8_t tabs) override {
        /*static const std::unordered_set<std::string> types {"char", "short", "int", "long"};
        static const std::unordered_set<std::string> keywords {"if", "else", "return", "throw"};


        tokens.reserve(tokens.size() + new_tokens.size() + 1);

        {
            token_t t {"0", NEWL};
            t.str[0] = tabs;
            tokens.push_back(t);
        }

        for (const modoc::string_type& s : new_tokens) {
            token_t t;
            t.str = s.view();

            if (types.contains(t.str)) t.type = TYPE;
            else if (keywords.contains(t.str)) t.type = KEYWORD;

            tokens.push_back(t);
        }*/
    }

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
    };

    static uint32_t argb(uint8_t type, const theme& t) {
        switch (type) {
            case KEYWORD: return t.keywords; 
            case TYPE: return t.types;
            case OPERATOR: return t.operators;
            case NUMBER: return t.numbers;
            case STRING: return t.strings;
            case FUNCTION: return t.functions;
        }
        return t.text;
    }

    //TODO: Could be combined with lsp modoc tokenize????
    static token_t tokenize_value(std::string_view str, const theme& theme) {
        static const std::unordered_set<std::string_view> types {"char", "short", "int", "long", "float", "double", "uint16_t"};
        static const std::unordered_set<std::string_view> keywords {"if", "else", "return", "throw", "for", "constexpr", "#include"};

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
                const char* const begin = ptr;
                while (ptr < str.end() && *ptr > ' ' && !modoc::operator_chars.contains(*ptr)) ++ptr;

                len = ptr - begin;

                if ((len == 4 && strncmp(begin, "true", 4) == 0) || (len == 5 && strncmp(begin, "false", 5) == 0)) type = BOOLEAN;
                else if (types.contains({begin, ptr})) type = TYPE;
                else if (keywords.contains({begin, ptr})) type = KEYWORD;
            }
        }

        return {{str.data(), str.data() + len}, (token_type)type, argb(type, theme)};
    }

    static std::vector<token_t> tokenize(std::string_view str, const theme& theme) {
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
                const token_t t = tokenize_value({ptr, str.end()}, theme);
                std::cout << t.str << '\n';
                result.push_back(t);
                ptr += t.str.size() - 1;
            }
            else if (*ptr == '\n') result.emplace_back(std::string_view{ptr, ptr + 1}, NEWL, 0);
        }

        if (begin != nullptr) {
            const std::string_view view = {begin, str.end()};
            result.push_back(tokenize_value(view, theme));
            
            /*if (types.contains(view)) result.emplace_back(view, TYPE);
            else if (keywords.contains(view)) result.emplace_back(view, KEYWORD);
            else result.emplace_back(view, NONE);*/
        }

        {
            modoc::lsp_process lsp;
            const std::string_view name = "file:///tmp/main.cpp";

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
                    if (lsp.token_types[itr2->type_id] == "function") itr1 = result.insert(itr1, token_t{itr2->str, FUNCTION, argb(FUNCTION, theme)});
                    else if (lsp.token_types[itr2->type_id] == "operator") itr1 = result.insert(itr1, token_t{itr2->str, OPERATOR, argb(OPERATOR, theme)});
                    ++itr2;
                }
                else if (itr2->str.begin() == itr1->str.begin()) {
                    std::cout << '=' << itr1->str << ' ' << itr2->str << '\n';
                    if (lsp.token_types[itr2->type_id] == "function") *itr1 = {itr2->str, FUNCTION, argb(FUNCTION, theme)};
                    else if (lsp.token_types[itr2->type_id] == "operator") *itr1 = {itr2->str, OPERATOR, argb(OPERATOR, theme)};
                    ++itr2;
                }
                ++itr1;
            }

            while (itr2 < tokens.end()) {
                if (lsp.token_types[itr2->type_id] == "function") result.emplace_back(itr2->str, FUNCTION, argb(FUNCTION, theme));
                else if (lsp.token_types[itr2->type_id] == "operator") result.emplace_back(itr2->str, OPERATOR, argb(OPERATOR, theme));
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
        }

        return t;
    }
    
    void parse_verbatim(std::string_view str, bool to_copy) override {
        if (!meta.contains("padding")) meta["padding"] = 6;

        modoc::logger::s_log("code", "verbatim", str);
        content = str;

        tokens = tokenize(content, get_theme());
        
        std::string result;
        for (auto& t : tokens) {
            result += '(';
            result += t.str;
            result += ", ";
            result += std::to_string(t.type);
            result += ')';
        }
        modoc::logger::s_log("code", "verbatim", result);
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
        if (op.contains("lang") && op.at("lang").type() == value::STRING) {
            return new code_node((std::string)op.at("lang").string());
        }
        return nullptr;
    }

    void set_node_type_id(uint32_t id) const override {
        code_node::t_id = id;
    }
};
