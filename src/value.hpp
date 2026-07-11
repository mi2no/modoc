#include <cstdlib>
#include <string_view>
#include <span>

#include "../../serialize/serialize.hpp"

struct value {

    enum type_e : uint8_t {
        NONE, BOOLEAN, NUMBER, STRING, LIST
    };

    struct list_data {
        value* ptr;
        size_t size;
    };


    union value_data {
        bool boolean;
        double number;
        struct {
            const char* ptr;
            size_t size;
            bool copy;
        } string;
        list_data list;

        value_data() : boolean(false) {}
    } data;

    type_e type = NONE;

    static value _parse(const char*& str) {
        value result;

        while (*str == ' ' || *str == '\n') ++str;

        if (str[0] == '"') {
            result.type = STRING;
            result.data.string = {++str, 0, false};

            while (*str++ != '"') ++result.data.string.size;
        }
        else if (str[0] == '[') {
            result.type = LIST;
            list_data& list = result.data.list = {nullptr, 1};
            
            {
                const char* ptr = str;
                while (*++ptr != ']')
                    if (*ptr == ',') ++list.size;

                list.ptr = new value[list.size];
            }

            size_t init = 0;
            ++str;
            while (*str != ']') {
                list.ptr[init] = _parse(str);
                if (list.ptr[init].type != NONE) ++init;
                
                while (*str != ',' && *str != ']') ++str;
                if (*str == ',') ++str;
            }
        }
        else {
            char* end;
            const double num = strtod(str, &end);

            if (str != end && (*end == ']' || *end == ' ' || *end == ',' || *end == '\0')) {
                result.type = NUMBER;
                result.data.number = num;
                str = end;
            }

            //TODO: check if is a static const
        }

        return result;
    }

    static value parse(const char* str) {
        return _parse(str); 
    }

    std::string_view string() const {
        return {data.string.ptr, data.string.ptr + data.string.size};
    }

    std::span<value> list() const {
        return {data.list.ptr, data.list.ptr + data.list.size};
    }

    ~value() {
        if (type == LIST) delete[] data.list.ptr;
        else if (type == STRING && data.string.copy) delete[] data.string.ptr;
    }
};
