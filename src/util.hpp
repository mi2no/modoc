#pragma once

#include "log.hpp"
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

    static std::string remove_indent(std::string_view str, const uint8_t count) {
        std::string result;

        modoc::logger::s_log("ri", "count", std::to_string(count));

        const char* ptr = str.data();
        while (ptr < str.end()) {
            { // Skip 'count' tabs
                uint8_t c = 0;
                while (c < count && *ptr == '\t') {
                    ++c;
                    ++ptr;
                }
            }
            const char* begin = ptr;

            while (ptr < str.end() && *ptr != '\n') ++ptr;

            if (ptr < str.end()) ++ptr;
            
            result += {begin, ptr}; 
            modoc::logger::s_log("ri", "line", {begin, ptr});
        }

        return result;
    }

    static consteval size_t string_len(const char* str) {
        size_t size = 0;
        while (str[size] != '\0') ++size;
        return size;
    }
};
