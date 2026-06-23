#pragma once
#include <cstdint>
#include <stdint.h>
#include <map>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "options.hpp"

enum scope_end : uint8_t {
    START, ENDL, ENDSCP
};

typedef std::unordered_set<std::string> dependencies_t;

struct keyword_instance {
    const char *begin = "", *end = "";
    uint8_t data = 0b100u | ENDSCP;

    virtual std::string format(const std::vector<std::string_view>& tokens, dependencies_t&) = 0;
    virtual ~keyword_instance() = default;

    void set_scope_end(const scope_end& s) { data = data & 0b11111100u | s; }
    void set_process_tokens(const bool& b) { data = data & 0b11111011u | (b << 2); }

    uint8_t scope_end() const { return data & 0b11u; }
    bool process_tokens() const { return data & 0b100u; }
};

struct keyword {
    virtual keyword_instance* get_instance(const uint8_t& nesting, const options_t& options) const = 0;
};

struct inline_keyword {
    const char* begin;
    bool single;
};

struct enum_instance : keyword_instance {

    const uint8_t nesting;
    
    enum_instance(const uint8_t& nesting) : nesting(nesting) {
        begin = "\\begin{enumerate}";
        end = "\\end{enumerate}";
    }

    std::string format(const std::vector<std::string_view>& tokens, dependencies_t&) {
        std::string result;

        bool item = false, line_start = true;
        for (size_t i = 0; i < tokens.size(); ++i) {
            const std::string_view& view = tokens[i];

            if (view.begin() == nullptr) {
                result += '\n';
                item = false;
                line_start = true;
            }
            else {
                if (line_start)  {
                    for (uint8_t n = 0; n <= nesting; ++n) result += '\t';
                    line_start = false;
                }
                if (view[0] == '-' && !item) {
                    result += "\\item";
                    result += ' ';
                    result.append(view.begin() + 1, view.size() - 1);
                    item = true;
                }
                else result += view;
                result += ' ';
            }
        }

        return result;
    }
};

struct enum_k : keyword {
    keyword_instance* get_instance(const uint8_t& nest, const options_t& opt) const {
        //if (opt.contains("smbl") && opt.at("smbl").view == "+") return new enum_instance(nest, "\\xd");
        return new enum_instance(nest);
    }
};


struct sec_instace : keyword_instance {

    sec_instace(const uint8_t& nest) {
        switch (nest) {
            case 1:
                begin = "\\subsection";
                break;
            case 2:
                begin = "\\subsubsection";
                break;
            default:
                begin = "\\section";
        }
        set_scope_end(ENDL);
    }

    std::string format(const std::vector<std::string_view>& tokens, dependencies_t&) {
        std::string result = "{";
        
        for (size_t i = 0; i < tokens.size(); ++i) {
            result += tokens[i];
            if (i < tokens.size() - 1) result += ' ';
        }
        result += '}';

        return result;
    }

};

struct sec_k : keyword {
    keyword_instance* get_instance(const uint8_t& nest, const options_t&) const {
        return new sec_instace(nest);
    }
};

#include <stack>

struct math_instance : keyword_instance {

    math_instance() {
        begin = "\\[";
        end = "\\]";
    }

    std::string format(const std::vector<std::string_view>& tokens, dependencies_t&) {
        struct line {
            std::string s;
            bool close = false;
        };
        std::stack<line> stack;
        std::string_view prev = tokens[0];
        bool frac = false;

        stack.push({""});

        for (size_t i = 1; i < tokens.size(); ++i) {
            std::string& result = stack.top().s;

            if (tokens[i].begin() == nullptr) {
                result += '\n';
                continue;
            }

            if (tokens[i] == "/") {

                result += "\\frac{";
                result += prev;
                result += "}{";
                stack.top().close = true;
                ++i;
            }
            else {
                result += prev;
                if (stack.top().close) {
                    result += '}';
                    stack.top().close = false;
                }
                result += ' ';
            }
            prev = tokens[i];

            if (tokens[i][0] == '(') stack.push({""});
            else if (tokens[i].back() == ')') {
                result += tokens[i];
                prev = result;
                stack.pop();
            }

            //printf("%zu %zu\n", i, tokens.size());
            //std::cout << stack.size() << '\n';
            //std::cout << result << '\n' << stack.top() << '\n';
        }

        stack.top().s += prev;
        if (stack.top().close) stack.top().s += '}';
        return stack.top().s;
    }

};

struct math_k : keyword {
    keyword_instance* get_instance(const uint8_t&, const options_t&) const {
        return new math_instance();
    }
};
