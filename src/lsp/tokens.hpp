#pragma once
#include <string>
#include <string_view>
#include <cstdint>
#include <vector>

struct token {
    enum token_e : uint8_t {
        KEYWORD, NONE
    };

    uint32_t line;
    uint32_t col;
    uint32_t len;
    uint8_t type;
};

static std::vector<token> tokenize(/*std::string_view*/ const char* str) {
    std::vector<token> result{};

    token t {0, 0, 0};
    const char* begin = str;
    while (*str != '\0') {
        while (*str == ' ' || *str == '\n' || *str == '[' || *str == ']') {
            if (*str == '\n') {
                ++t.line;
                begin = str;
            }
            ++str;
        }

        t.col = str - begin;

        if (*str == '@') t.type = token::KEYWORD;
        else t.type = token::NONE;

        while (*str != ' ' && *str != '[' && *str != ']' && *str != '\n' && *str != '\0') ++str;

        if (t.type != token::NONE) {
            t.len = str - begin + t.col;
            result.push_back(t);
        }
    }

    return result;
}

static std::string tokenize_json(const char* str, uint32_t id) {
    std::vector<token> tokens = tokenize(str);
    std::string json = std::string("{\"jsonrpc\":\"2.0\",\"id\":") + std::to_string(id) + ",\"result\":{\"data\":[";

    for (size_t i = 0; i < tokens.size(); ++i) {
        const token& t = tokens[i];

        json += std::to_string(t.line) + ',';
        json += std::to_string(t.col) + ',';
        json += std::to_string(t.len) + ',';
        json += std::to_string((uint16_t)t.type) + ",0";

        if (i < tokens.size() - 1) json += ',';
    }

    json += "]}}";
    return json;
}
