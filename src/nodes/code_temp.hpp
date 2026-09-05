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

        token_t(std::string_view str, token_type type) : str(str), type(type), color(argb(type)) {}
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

    static uint32_t argb(uint8_t type, value::object_t* theme = nullptr) {
        switch (type) {
            case KEYWORD: return theme == nullptr ? 0xFF8839ef : theme->at("keyword").number();
            case TYPE: return theme == nullptr ? 0xFFdf8e1d : theme->at("type").number();
            case OPERATOR: return theme == nullptr ? 0xFF04a5e5 : theme->at("operator").number();
            case NUMBER: return theme == nullptr ? 0xFFfe640b : theme->at("number").number();
            case STRING: return theme == nullptr ? 0xFF40a02b : theme->at("string").number();
            case FUNCTION: return theme == nullptr ? 0xFF1e66f5 : theme->at("function").number();
        }
        return theme == nullptr ? 0xFF4c4f69 : theme->at("text").number();
    }

    //TODO: Could be combined with lsp modoc tokenize????
    static token_t tokenize_value(std::string_view str, value::object_t* theme = nullptr) {
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

    static std::vector<token_t> tokenize(std::string_view str, value::object_t* theme = nullptr) {
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
            else if (*ptr == '\n') result.emplace_back(std::string_view{ptr, ptr + 1}, NEWL);
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
    
    void parse_verbatim(std::string_view str, bool to_copy) override {
        if (!meta.contains("theme")) meta["theme"] = constants.at("code").object().at("theme").object().at("latte");
        modoc::logger::s_log("code", "meta", this->meta.at("theme").to_string());

        modoc::logger::s_log("code", "verbatim", str);
        content = str;

        tokens = tokenize(content, &meta["theme"].object());
        
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
    static value::object_t themes() {
        value::object_t map;

        map["latte"] = value::from_object({
            {"background", 0xFFeff1f5}, // Base
            {"lineNumbers", 0xFF8c8fa1}, // Overlay 1
            {"text", 0xFF4c4f69}, // Text
            {"keyword", 0xFF8839ef}, // Mauve
            {"type", 0xFFdf8e1d}, // Yellow
            {"operator", 0xFF04a5e5}, // Sky
            {"number", 0xFFfe640b}, // Peach
            {"string", 0xFF40a02b}, // Green
            {"function", 0xFF1e66f5} // Blue
        });

        map["frappe"] = value::from_object({
            {"background", 0xFF303446}, // Base
            {"lineNumbers", 0xFF838ba7}, // Overlay 1
            {"text", 0xFFc6d0f5}, // Text
            {"keyword", 0xFFca9ee6}, // Mauve
            {"type", 0xFFe5c890}, // Yellow
            {"operator", 0xFF99d1db}, // Sky
            {"number", 0xFFef9f76}, // Peach
            {"string", 0xFFa6d189}, // Green
            {"function", 0xFF8caaee} // Blue
        });

        return map;
    }

    void init() override {
        value obj = value::from_object({});
        value::object_t& map = obj.object();

        map["theme"] = value::from_object(themes());

        {
            map["lang"] = value::from_object({
                {"cpp", value("cpp")}
            }); 
        }

        register_constant("code", std::move(obj));
    }

    node* instance(uint8_t, const options_t& op) override {
        if (op.contains("lang") && op.at("lang").type() == value::STRING) {
            return new code_node((std::string)op.at("lang").string());
        }
        return nullptr;
    }

    void set_node_type_id(uint32_t id) const override {
        code_node::t_id = id;
    }
};
