#pragma once

#include <algorithm>
#include <alloca.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <string_view>

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

    private:

        /*std::vector<std::string> split(std::string_view str) {
            std::vector<std::string> result;
            std::string line;

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

            

            return result;
        }*/

    public:

        std::string to_string(std::string_view name, std::string_view content, type_t type = LOG) const {
            std::string result;

            { // name frame ╭───────────────────────╮
                const uint16_t frame_width = std::min((size_t)width - 2, name.size() + 3 /*type*/ + 3 /*spacer*/ + 2 /*frame*/ + 2 /*padding*/);
                result.reserve(frame_width * strlen(LINE_VERTICAL) + 1);
                result += ' ';
                result += CORNER_ROUND_TOP_LEFT;
                for (uint16_t i = 0; i < frame_width - 2; ++i) result += LINE_HORIZONTAL;
                result += CORNER_ROUND_TOP_RIGHT;
                result += '\n';

                result += CORNER_ROUND_TOP_LEFT;
                result += CORNER_ROUND_BOTTOM_RIGHT;
                result += ' ';
                result += name;
                result += " | ";
                result += "LOG";
                result += ' ';
                result += CORNER_ROUND_BOTTOM_LEFT;
                for (uint16_t i = frame_width + 1; i < width; ++i) {
                    result += LINE_HORIZONTAL;
                }
                result += CORNER_ROUND_TOP_RIGHT;
                result += '\n';

                // |------|
                result += LINE_VERTICAL;
                for (uint16_t i = 0; i < width - 2; ++i) result += '-';
                result += LINE_VERTICAL;
                result += '\n';
            }

            return result;
        }

        void log(std::string_view name, std::string_view content, type_t type = LOG) const {
            char* buff = (char*)alloca(width + 2 + 4);
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
            fputs(CORNER_ROUND_BOTTOM_RIGHT, out);
        }
    };
}

/*
 * 
 *╭▀▀▀▀
 *
 ╭───────────────────────╮
╭╯ modoc : summary │ LOG ╰────╮
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
