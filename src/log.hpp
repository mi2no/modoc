#pragma once

#include <algorithm>
#include <alloca.h>
#include <bit>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <string_view>


#include <iostream>

namespace modoc {
    struct logger {
        FILE* out = stdout;
        uint16_t width = 30;

        enum type_t : uint8_t {
            LOG, ERR, WARN
        };

        static constexpr const char* LINE_VERTICAL = "\u2502";
        static constexpr const char* LINE_HORIZONTAL = "\u2500";
        static constexpr const char* CORNER_ROUND_TOP_LEFT = "\u256D";
        static constexpr const char* CORNER_ROUND_TOP_RIGHT = "\u256E";
        static constexpr const char* CORNER_ROUND_BOTTOM_LEFT = "\u2570";
        static constexpr const char* CORNER_ROUND_BOTTOM_RIGHT = "\u256F";

        static constexpr const char* BULLET_CIRCLE = "\u2022";
        
        static constexpr const char* INNER_LEFT = "\u255F";
        static constexpr const char* INNER = "\u254C";
        static constexpr const char* INNER_RIGHT = "\u2562";

        static constexpr uint8_t char_width(uint8_t c) {
            const uint8_t n = std::countl_one(c);
            return std::max(n, (uint8_t)1u);
        }

    private:

        static std::vector<std::string> fit(std::string_view str, uint16_t width) {
            std::vector<std::string> result;

            const char *ptr = str.data(), *begin = ptr;
            uint16_t visual_width = 0;
            while (ptr < str.end()) {
                if (*ptr == '\n') {
                    std::string line = {begin, ptr};
                    line.reserve(width);
                    while (visual_width++ < width) line += ' ';
                    result.push_back(std::move(line));
                    begin = ptr + 1;
                    visual_width = 0;
                }
                else if (visual_width == width) {
                    result.emplace_back(begin, ptr);
                    begin = ptr;
                    const uint8_t n = char_width(*ptr);
                    ptr += n - 1;
                    visual_width = 1;
                }
                else {
                    const uint8_t n = char_width(*ptr);
                    ptr += n - 1;
                    ++visual_width;
                }
                ++ptr;
            }
            if (ptr - begin > 0) {
                std::string line = {begin, ptr};
                line.reserve(width);
                while (line.size() < width) line += ' ';
                result.push_back(std::move(line));
                begin = ptr + 1;
            }

            return result;
        }

    public:

        std::string to_string(std::string_view context, std::string_view title, std::string_view content, type_t type = LOG) const {
            std::string result;

            const std::string color = "\033[32m";

            { // name frame ╭───────────────────────╮
                const uint16_t frame_width = std::min((size_t)width - 2, context.size() + title.size() + 3 /*type*/ + 3 /*spacer*/ + 2 /*frame*/ + 2 /*padding*/ + 3 /* . */);
                result.reserve(frame_width * strlen(LINE_VERTICAL) + 1);
                result += ' ';
                result += color;
                result += CORNER_ROUND_TOP_LEFT;
                for (uint16_t i = 0; i < frame_width - 2; ++i) result += LINE_HORIZONTAL;
                result += CORNER_ROUND_TOP_RIGHT;
                result += "\033[0m";
                result += '\n';

                result += color;
                result += CORNER_ROUND_TOP_LEFT;
                result += CORNER_ROUND_BOTTOM_RIGHT;
                result += "\033[0m";
                result += ' ';
                result += "\033[1m";
                result += context;
                result += ' ';
                result += BULLET_CIRCLE; 
                result += ' ';
                result += title;
                result += "\033[0m";
                result += color;
                result += " | ";
                result += "\033[0m";
                result += "\033[1m";
                result += "LOG";
                result += "\033[0m";
                result += ' ';
                result += color;
                result += CORNER_ROUND_BOTTOM_LEFT;
                for (uint16_t i = frame_width + 2; i < width; ++i) {
                    result += LINE_HORIZONTAL;
                }
                result += CORNER_ROUND_TOP_RIGHT;
                result += "\033[0m";
                result += '\n';

                // |------|
                result += color;
                result += INNER_LEFT;
                for (uint16_t i = 0; i < width - 2; ++i) result += '-';
                result += INNER_RIGHT;
                result += "\033[0m";
                result += '\n';
            }

            // Content
            {
                std::vector<std::string> lines = fit(content, width - 4);
                for (std::string_view view : lines) {
                    result.reserve(result.size() + view.size() + 8 + 2 * color.size());
                    result += color;
                    result += LINE_VERTICAL;
                    result += "\033[0m";
                    result += ' ';
                    result += view;
                    result += ' ';
                    result += color;
                    result += LINE_VERTICAL;
                    result += "\033[0m";
                    result += '\n';
                }
            }

            // Bottom
            {
                result += color;
                result.reserve(width * 3);
                result += CORNER_ROUND_BOTTOM_LEFT;
                for (uint16_t i = 0; i < width - 2; ++i) result += LINE_HORIZONTAL;
                result += CORNER_ROUND_BOTTOM_RIGHT;
                result += '\n';
                result += "\033[0m";
            }

            return result;
        }

        void log(std::string_view context, std::string_view title, std::string_view content, type_t type = LOG) const {
            /*char* buff = (char*)alloca(width + 2 + 4);
            memcpy(buff, LINE_VERTICAL, 3);
            memcpy(buff + width + 1, LINE_VERTICAL, 3);
            buff[width + 4] = '\n';
            buff[width + 5] = '\0';
            memset(buff + 3, ' ', width - 2);

            char* buff_ptr = buff + 4;
            for (const char* ptr = content.data(); ptr < content.end(); ++ptr) {
                if (*ptr == '\n' || buff_ptr - buff == width - 2) {
                    fputs(buff, out);        
                    memset(buff + 3, ' ', width - 2);
                    buff_ptr = buff + 4;
                }
                if (*ptr != '\n') *buff_ptr++ = *ptr;
            }
            if (buff_ptr - buff > 4) fputs(buff, out);
        
            fputs(CORNER_ROUND_BOTTOM_LEFT, out);
            for (uint16_t i = 0; i < width - 2; ++i) fputs(LINE_HORIZONTAL, out);
            fputs(CORNER_ROUND_BOTTOM_RIGHT, out);*/
            std::string result = to_string(context, title, content);
            fputs(result.c_str(), out);
        }
    };
}

/*
 * 
 *╭▀▀▀▀
 *
 ╭───────────────────────╮
╭╯ modoc • summary │ LOG ╰────╮
│-----------------------------│
│ errors                    0 │
│ time                  9.32s │
╰─────────────────────────────╯

 ╭─────────────────╮
╭╯ modoc : summary ╰───────────╮
│L ----------------------------│
│O errors                    0 │
│G time                  9.32s │
╰╮                             │
 ╰─────────────────────────────╯
 *
 * */
