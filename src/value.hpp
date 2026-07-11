#include <cstdlib>
#include <cstring>
#include <string_view>
#include <span>
#include <map>

#include "../../serialize/serialize.hpp"

struct value {

    enum type_e : uint8_t {
        NONE, BOOLEAN, NUMBER, STRING, LIST
    };

    struct string_data {
        const char* ptr;
        size_t size;
        bool copy;
    }; // No terminating char at string end ('\0')!

    struct list_data {
        value* ptr;
        size_t size;
    };

    union value_data {
        bool boolean;
        double number;
        string_data string;
        list_data list;

        value_data() : boolean(false) {}
    } data;

    type_e type = NONE;

    value() = default;

    // Copy constructor
    value(const value& v) {
        type = v.type;
        data = v.data;
        
        if (type == STRING && data.string.copy) {
            data.string.ptr = new char[data.string.size];
            memcpy((char*)data.string.ptr, v.data.string.ptr, data.string.size);
        }
        else if (type == LIST) {
            data.list.ptr = new value[data.list.size];
            for (size_t i = 0; i < data.list.size; ++i) data.list.ptr[i] = v.data.list.ptr[i];
        }
    }

    value& operator=(const value& v) {
        type = v.type;
        data = v.data;
        
        if (type == STRING && data.string.copy) {
            data.string.ptr = new char[data.string.size];
            memcpy((char*)data.string.ptr, v.data.string.ptr, data.string.size);
        }
        else if (type == LIST) {
            data.list.ptr = new value[data.list.size];
            for (size_t i = 0; i < data.list.size; ++i) data.list.ptr[i] = v.data.list.ptr[i];
        }

        return *this;
    }



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
            ++str;
        }
        else {
            char* end = (char*)str + 4;
            
            if (strncmp(str, "true", 4) == 0 && (*end == ']' || *end == ' ' || *end == ',' || *end == '\0')) {
                result.type = BOOLEAN;
                result.data.boolean = true;
                str = end;
                return result;
            }

            ++end;
            if (strncmp(str, "false", 5) == 0 && (*end == ']' || *end == ' ' || *end == ',' || *end == '\0')) {
                result.type = BOOLEAN;
                result.data.boolean = false;
                str = end;
                return result;
            }
            
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

    bool boolean() const {
        return data.boolean;
    }

    double number() const {
        return data.number;
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

typedef std::map<std::string_view, value> options_t;

static void parse_options(const char* ptr, options_t& ops) {
    std::string_view option;
    const char* begin = nullptr;
    bool is_value = false;

    ++ptr;
    while (*ptr != ']' && *ptr != '\0') {
        if (!is_value) {
            if (*ptr > ' ' && begin == nullptr) {
                begin = ptr;
            }
            else if (begin != nullptr && (*ptr <= ' ' || *ptr == '=')) {
                option = {begin, ptr};
                begin = nullptr;
                is_value = true;
            }
        }
        else if (*ptr > ' ' && *ptr != '=') {
            ops[option] = value::_parse(ptr);
            is_value = false;
            if (*ptr == ']' || *ptr == '\0') break;
        }
        ++ptr;
    }
}
