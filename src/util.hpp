#pragma once

#include <set>
#include <string_view>
#include <cstdint>

namespace modoc {

    static std::set<char> operator_chars = {'(', ')', '{', '}', '[', ']', '+', '-', '*', '/', '^', '=', ',', '.'};

    static std::string_view get_scope(std::string_view str, const char open, const char close) {
        const char* ptr = str.data();

        while (ptr < str.end() && *ptr != open) ++ptr;
        if (ptr == str.end()) return {};

        const char* const start = ptr;
        uint8_t depth = 1;

        while (depth) {
            ++ptr;
            if (*ptr == open) ++depth;
            else if (*ptr == close) --depth;
        }

        return depth ? std::string_view{} : std::string_view{start + 1, ptr};
    }
};
