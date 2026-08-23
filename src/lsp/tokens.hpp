#pragma once
#include <string>
#include <string_view>
#include <cstdint>
#include <vector>

struct token {
    enum token_e : uint8_t {
        KEYWORD, OPERATOR, STRING, NUMBER, COMMENT, VARIABLE, NONE
    };

    uint32_t line;
    uint32_t col;
    uint32_t len;
    uint8_t type;
};

static std::vector<token> tokenize(std::string_view str) {
    std::vector<token> result{};

    token t {0, 0, 0};
    const char *begin = str.data(), *ptr = str.data();
    while (ptr < str.end()) {
        if (*ptr == '\n') {
            ++t.line;
            t.col = 0;
            ++ptr;
        }
        else if (*ptr == '@') {
            t.type = token::KEYWORD;
            begin = ptr;

            while (ptr < str.end() && *ptr > ' ' && *ptr != '[' && *ptr != '(' && *ptr != '{' && *ptr != '$') ++ptr;

            t.len = ptr - begin;
            
            result.push_back(t);

            t.col += t.len;
        }
        else if (*ptr == '$') {
            t.type = token::OPERATOR;

            t.len = 1;
            result.push_back(t);

            ++ptr;
            ++t.col;
        }
        else if (*ptr > ' ') {
            t.type = token::NONE;
            begin = ptr;

            while (ptr < str.end() && *ptr > ' ' && *ptr != '$') ++ptr;

            t.len = ptr - begin;

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
