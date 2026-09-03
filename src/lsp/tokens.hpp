#pragma once
#include <charconv>
#include <cstring>
#include <set>
#include <string>
#include <string_view>
#include <cstdint>
#include <tuple>
#include <vector>

#include "log.hpp"
#include "../util.hpp"

struct token {
    enum token_e : uint8_t {
        KEYWORD, OPERATOR, STRING, NUMBER, COMMENT, VARIABLE, PROPERTY, PARAMETER, BOOLEAN, NODE, NONE
    };

    uint32_t line;
    uint32_t col;
    uint32_t len;
    uint8_t type;

    const char* ptr;
};

static std::set<char> operators = {'(', ')', '[', ']', '{', '}', '+', '-', '*', '/', '^', '=', ','};

static std::tuple<uint8_t, uint32_t> tokenize_value(std::string_view str) {
    const char* ptr = str.data();
    uint8_t type = token::NONE;
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
        
        type = token::STRING;
        len = ptr - begin;
    }
    else {
        double num;
        auto [read_end, ec] = std::from_chars(ptr, str.end(), num);

        if (ptr != read_end && ec == std::errc{}) {
            type = token::NUMBER;
            len = read_end - ptr;
        }
        else {
            const char* const begin = ptr;
            while (ptr < str.end() && *ptr > ' ' && !operators.contains(*ptr)) ++ptr;

            len = ptr - begin;

            if ((len == 4 && strncmp(begin, "true", 4) == 0) || (len == 5 && strncmp(begin, "false", 5) == 0)) type = token::BOOLEAN;
            else type = token::PROPERTY;
        }
    }

    return {type, len};
}

static const char* tokenize_expression(std::string_view str, uint32_t& line, uint32_t& col, std::vector<token>& v, char end) {
    const char* ptr = str.data();
    uint16_t depth = 0;

    while (ptr < str.end()) {
        log.f("[EXPR] Line: %u, col: %u\n", line, col);
        if (*ptr == '(') {
            ++depth;
            v.emplace_back(line, col, 1, token::OPERATOR, ptr);
            ++ptr;
            ++col;
        }
        else if (*ptr == ')') {
            --depth;
            v.emplace_back(line, col, 1, token::OPERATOR, ptr);
            ++ptr;
            ++col;
            if (end == ')' && depth == 0) break;
        }
        else if (*ptr == end) break;
        else if (*ptr == '\n') {
            ++line;
            col = 0;
            ++ptr;
        }
        else if (*ptr <= ' ') {
            ++ptr;
            ++col;
        }
        else if (*ptr == '+' || *ptr == '-' || *ptr == '*' || *ptr == '/' || *ptr == '^') {
            v.emplace_back(line, col, 1, token::OPERATOR, ptr);
            ++ptr;
            ++col;
        }
        else {
            auto [type, len] = tokenize_value({ptr, str.end()});

            if (type != token::NONE) v.emplace_back(line, col, len, type, ptr);
            else len = 1;
            
            ptr += len;
            col += len;
        }
    }
    return ptr;
}

static const char* tokenize_parameters(std::string_view scope, uint32_t& line, uint32_t& col, std::vector<token>& v) {
    const char* ptr = scope.data();
    uint8_t mode = 0; // 0 - parameter, 1 - operator (=), 2 - expression

    while (ptr < scope.end()) {
        if (*ptr > ' ') {
            if (mode == 2) {
                ptr = tokenize_expression({ptr, scope.end()}, line, col, v, ',');
                log.f("[PARAM] back: %.*s\n", (int)v.back().len, v.back().ptr);
                if (ptr < scope.end()) { // TODO: fix
                    ++ptr;
                    ++col;
                }
            }
            else {
                const char* begin = ptr;
                while (ptr < scope.end() && *ptr > ' ') ++ptr;

                const uint32_t len = ptr - begin;
                v.emplace_back(line, col, len, mode ? token::OPERATOR : token::PARAMETER, begin);
                col += len;
            }

            mode = (mode + 1) % 3;
        }
        else if (*ptr == '\n') {
            ++line;
            col = 0;
            ++ptr;
        }
        else {
            ++ptr;
            ++col;
        }
    }

    return ptr;
}


static std::vector<token> tokenize(std::string_view str) {
    std::vector<token> result{};

    token t {0, 0, 0};
    const char *begin = str.data(), *ptr = str.data();
    while (ptr < str.end()) {
        log.f("Line: %u, col: %u\n", t.line, t.col);
        if (*ptr == '\n') {
            ++t.line;
            t.col = 0;
            ++ptr;
        }
        else if (*ptr == '@') {
            t.type = token::NODE;
            begin = ptr;

            while (ptr < str.end() && *ptr > ' ' && *ptr != '[' && *ptr != '(' && *ptr != '{' && *ptr != '$') ++ptr;

            t.len = ptr - begin;
            t.ptr = begin;
            
            result.push_back(t);

            t.col += t.len;

            while (*ptr > ' ') {
                if (*ptr == '{') {
                    result.emplace_back(t.line, t.col, 1, token::OPERATOR, ptr);

                    std::string_view scope = modoc::get_scope({ptr, str.end()}, '{', '}');
                    ++ptr;
                    ++t.col; 
                    
                    ptr = tokenize_parameters(scope, t.line, t.col, result);

                    result.emplace_back(t.line, t.col, 1, token::OPERATOR, ptr);
                }
                else if (*ptr == '[') {
                    result.emplace_back(t.line, t.col, 1, token::OPERATOR, ptr);
                    
                    std::string_view scope = modoc::get_scope({ptr, str.end()}, '[', ']');
                    ++ptr;
                    ++t.col;
                     
                    ptr = tokenize_parameters(scope, t.line, t.col, result);

                    result.emplace_back(t.line, t.col, 1, token::OPERATOR, ptr);
                }

                ++ptr;
                ++t.col;
            }
        }
        else if (*ptr == '$') {
            t.type = token::OPERATOR;

            t.len = 1;
            result.push_back(t);

            ++ptr;
            ++t.col;

            if (*ptr != '(') {
                const auto [type, len] = tokenize_value({ptr, str.end()});

                if (type != token::NONE) {
                    result.emplace_back(t.line, t.col, len, type, ptr);
                    t.col += len;
                    ptr += len;
                }
            }
            else {
                ptr = tokenize_expression({ptr, str.end()}, t.line, t.col, result, ')');
            }
        }
        else if (*ptr > ' ') {
            t.type = token::NONE;
            begin = ptr;

            while (ptr < str.end() && *ptr > ' ' && *ptr != '$') ++ptr;

            t.len = ptr - begin;
            t.ptr = begin;

            result.push_back(t);

            t.col += t.len;
        }
        else {
            ++ptr;
            ++t.col;
        }
    }

    return result;
}

static std::string tokenize_json(const char* str, uint32_t id) {
    std::vector<token> tokens = tokenize(str);
    std::string json = std::string("{\"jsonrpc\":\"2.0\",\"id\":") + std::to_string(id) + ",\"result\":{\"data\":[";

    const token* prev = nullptr;
    for (size_t i = 0; i < tokens.size(); ++i) {
        const token& t = tokens[i];

        if (t.type == token::NONE) continue;

        log.f("[%.*s]", (int)t.len, t.ptr);

        uint32_t line = t.line, col = t.col;

        if (prev != nullptr) {
            json += ',';

            line -= prev->line;
            if (line == 0) col -= prev->col;
        }

        prev = &t;
        json += std::to_string(line) + ',';
        json += std::to_string(col) + ',';
        json += std::to_string(t.len) + ',';
        json += std::to_string((uint16_t)t.type) + ",0";
    }

    json += "]}}";
    return json;
}
