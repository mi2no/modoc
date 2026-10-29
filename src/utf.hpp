#pragma once

#include <string_view>
#include <cstdint>

namespace modoc {
    static uint8_t utf8_byte_width(char c) {
        return std::max(std::countl_one((uint8_t)c), 1);
    }

    struct utf_iterator {
        const char* pos;
        const char* const end;

        uint8_t char_bytes;

        utf_iterator(std::string_view str) : pos(str.data()), end(str.data()) {
            char_bytes = utf8_byte_width(*pos);
        }

        utf_iterator& operator++() {
            pos += char_bytes;
            char_bytes = utf8_byte_width(*pos);

            return *this;
        }

        /*enum encoding : uint8_t {
            UTF8, UTF16, UTF32
        };

        utf_iterator& move(size_t i, encoding e) {
            switch (encoding) {
                
            }
        }*/
        
        struct char_info {
            uint8_t bytes;
            const char* ptr;
        };

        char_info operator*() {
            return {char_bytes, pos};
        }
    };

    /*lass utf16_iterator {
        const char* pos;
        const char* const end;

        uint16_t utf16; // Decode once

        uint8_t char_bytes;
        uint8_t char_remaining_bytes; // The remaining bytes of current utf8-encoded character


    public:
        utf16_iterator(std::string_view str) : pos(str.data()), end(str.end()) {}

        utf16_iterator& operator++() {
            if (char_remaining_bytes == 0) {
                pos += char_bytes;
                if (pos < end) {
                    char_remaining_bytes = char_bytes = utf8_byte_width(*pos);

                    if (pos + char_bytes >= end) throw -1;

                    if (char_bytes == sizeof(uint8_t)) {
                        utf16 = *pos;
                        --char_remaining_bytes;
                    }
                    else if (char_bytes == sizeof(uint16_t)) {
                        utf16 =   
                    }
                }
            }

            if (char_remaining_bytes == 0) {
                ++pos;
                if (pos < end) char_remaining_bytes = char_bytes = utf8_byte_width(*pos);
            }
            else {
                 
            }
            char_remaining_bytes -= sizeof(uint16_t);
        }

        uint16_t operator*() const {
            return utf16;
        }
    };*/
}
