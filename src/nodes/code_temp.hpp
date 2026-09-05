#pragma once

#include <cstdint>

#include "../node.hpp"

struct code_node : node {
    inline static uint32_t t_id;

    enum token_type : uint8_t {
        NEWL, NONE, KEYWORD, TYPE, NUMBER, BOOLEAN, STRING, OPERATOR
    };

    struct token_t {
        std::string_view str;
        token_type type = NONE;
        uint32_t color;
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

    static uint32_t argb(uint8_t type) {
        switch (type) {
            case KEYWORD: return 0xFFC6A0F6u;
            case TYPE: return 0xFFeed49fu;
            case OPERATOR: return 0xFF91d7e3u;
            case NUMBER: return 0xFFf5a97fu;
        }
        return 0xFF000000u;
    }

    //TODO: Could be combined with lsp modoc tokenize????
    static token_t tokenize_value(std::string_view str) {
        static const std::unordered_set<std::string_view> types {"char", "short", "int", "long", "float", "double", "uint16_t"};
        static const std::unordered_set<std::string_view> keywords {"if", "else", "return", "throw", "for", "constexpr"};

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
            type = OPERATOR;
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
                //else type = PROPERTY;
            }
        }

        return {{str.data(), str.data() + len}, (token_type)type, argb(type)};
    }

    static std::vector<token_t> tokenize(std::string_view str) {
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
            if (*ptr > ' ') {
                const token_t t = tokenize_value({ptr, str.end()});
                std::cout << t.str << '\n';
                result.push_back(t);
                ptr += t.str.size() - 1;
            }

            if (*ptr == '\n') result.emplace_back(std::string_view{ptr, ptr + 1}, NEWL);
        }

        if (begin != nullptr) {
            const std::string_view view = {begin, str.end()};
            result.push_back(tokenize_value(view));
            
            /*if (types.contains(view)) result.emplace_back(view, TYPE);
            else if (keywords.contains(view)) result.emplace_back(view, KEYWORD);
            else result.emplace_back(view, NONE);*/
        }

        return result;
    }
    
    void parse_verbatim(std::string_view str, bool to_copy) override {
        modoc::logger::s_log("code", "verbatim", str);
        content = str;

        tokens = tokenize(content);
        
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

