#pragma once

#include <charconv>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <span>
#include <map>
#include <unordered_map>

#include <cstdio> //////

struct value;

static std::unordered_map<std::string_view, value> constants;

typedef std::map<std::string_view, value> options_t;

struct value {

    enum type_e : uint8_t {
        NONE, BOOLEAN, NUMBER, STRING, LIST
    };

    struct string_data {
        char* ptr;
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

    value(double num) {
        type = NUMBER;
        data.number = num;
    }

    value(std::string_view str) {
        type = STRING;
        data.string.size = str.size();
        data.string.ptr = new char[data.string.size];
        data.string.copy = true;
        memcpy(data.string.ptr, str.data(), data.string.size);
    }

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

    value(value&& v) {
        type = v.type;
        data = v.data;

        v.type = NONE;
        if (type == STRING) puts("str moved!");
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
    
    value& operator=(value&& v) {
        type = v.type;
        data = v.data;

        v.type = NONE;

        if (type == STRING) puts("str moved!");
        return *this;
    }


    void parse_string(const char* str, const char** end = nullptr) {
        char* ptr = (char*)str;
        uint32_t escape_chars = 0;
        bool slash = false;

        while (*++ptr != '"' || slash) {
            if (*ptr == '\\' && !slash) slash = true;
            else if (slash && (*ptr == 'n' || *ptr == '"' || *ptr == 't' || *ptr == '\\')) {
                slash = false;
                ++escape_chars;
            }
        }
        
        size_t size = ptr - str - 1;

        if (escape_chars) {
            data.string.size = size - escape_chars;
            ptr = data.string.ptr = new char[data.string.size];
            data.string.copy = true;
            slash = false;

            while (*++str != '"' || escape_chars) {
                if (*str == '\\' && !slash) slash = true;
                else if (slash) {
                    switch (*str) {
                        case 'n':
                            *ptr = '\n';
                            break;
                        case '"':
                            *ptr = '"';
                            break;
                        case 't':
                            *ptr = '\t';
                            break;
                        case '\\':
                            *ptr = '\\';
                            break;
                    }                    
                    slash = false;
                    --escape_chars;
                    ++ptr;
                }
                else *ptr++ = *str;
            }
        }
        else {
            data.string.ptr = (char*)str + 1;
            data.string.size = size;
            data.string.copy = false;
            str = ptr;
        }

        if (end != nullptr) *end = str;
    }

    static value _parse(const char*& str, std::function<const value*(std::string_view)> get_variable = nullptr) {
        value result;

        while (*str == ' ' || *str == '\n') ++str;

        if (str[0] == '"') {
            result.type = STRING;
            result.parse_string(str, &str);
            ++str;
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
                list.ptr[init] = _parse(str, get_variable);
                if (list.ptr[init].type != NONE) ++init;
                
                while (*str != ',' && *str != ']') ++str;
                if (*str == ',') ++str;
            }
            ++str;
        }
        else {
            char* end = (char*)str + 4;
            
            if (strncmp(str, "true", 4) == 0 && (*end == ']' || *end == ' ' || *end == ',' || *end == '\0' || *end == '}' || *end == '\n')) {
                result.type = BOOLEAN;
                result.data.boolean = true;
                str = end;
                return result;
            }

            ++end;
            if (strncmp(str, "false", 5) == 0 && (*end == ']' || *end == ' ' || *end == ',' || *end == '\0' || *end == '}' || *end == '\n')) {
                result.type = BOOLEAN;
                result.data.boolean = false;
                str = end;
                return result;
            }
            
            const double num = strtod(str, &end);
            if (str != end && (*end == ']' || *end == ' ' || *end == ',' || *end == '\0' || *end == '}' || *end == '\n')) {
                result.type = NUMBER;
                result.data.number = num;
                str = end;
                return result;
            }
            /*
             double num;
            const auto [end, ec] = std::from_chars(
                str,
                str + length,  // or whatever the end of your valid range is
                num
            );

            if (str != end &&
                ec == std::errc{} &&
                (end == str + length ||
                 *end == ']' || *end == ' ' || *end == ',' ||
                 *end == '}' || *end == '\n'))
            {
                result.type = NUMBER;
                result.data.number = num;
                str = end;
                return result;
            }
            */

            end = (char*)str;
            while (*end > ' ' && *end != ',' && *end != ']' && *end != '}') ++end;
            //printf("const: [%.*s]\n", (int)(end - str), str);

            std::string_view name = {str, end};
            str = end;

            const value* v = get_variable != nullptr ? get_variable(name) : nullptr;
            if (v != nullptr) {
                result = *v;
            }
            else if (constants.contains(name))
                result = constants.at(name);
        }

        return result;
    }

    static value parse(const char* str, std::function<const value*(std::string_view)> get_variable = nullptr) {
        return _parse(str, get_variable); 
    }

    static value parse(std::string_view view, const char** last = nullptr, std::function<const value*(std::string_view)> get_variable = nullptr) { //TODO: make this range-safe
        value result;
        const char *str = view.data(), * const end = view.end();

        while (str != end && (*str == ' ' || *str == '\n')) ++str;

        if (str == end) return result;

        if (str[0] == '"') {
            result.type = STRING;
            result.parse_string(str, &str);
            ++str;
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
                list.ptr[init] = _parse(str, get_variable);
                if (list.ptr[init].type != NONE) ++init;
                
                while (*str != ',' && *str != ']') ++str;
                if (*str == ',') ++str;
            }
            ++str;
        }
        else {
            //char* end = (char*)str + 4; // TODO: range
            const char* c = str + 4;
            
            if (end - str >= 4 && strncmp(str, "true", 4) == 0 && (*c == ']' || *c <= ' ' || *c == ',' || *c == '}' || *c == '\n')) {
                result.type = BOOLEAN;
                result.data.boolean = true;
                str = end;
                return result;
            }

            ++c;
            if (end - str >= 5 && strncmp(str, "false", 5) == 0 && (*c == ']' || *c <= ' ' || *c == ',' || *c == '}' || *c == '\n')) {
                result.type = BOOLEAN;
                result.data.boolean = false;
                str = end;
                return result;
            }
            
            /*const double num = strtod(str, &end);
            if (str != end && (*end == ']' || *end == ' ' || *end == ',' || *end == '\0' || *end == '}' || *end == '\n')) {
                result.type = NUMBER;
                result.data.number = num;
                str = end;
                return result;
            }*/
            
            double num;
            const auto [read_end, ec] = std::from_chars(str, end, num);

            if (str != read_end && ec == std::errc{} && (read_end == end || *read_end == ']' || *read_end == ' ' || *read_end == ',' || *read_end == '}' || *read_end == '\n')) {
                result.type = NUMBER;
                result.data.number = num;
                str = end;
                return result;
            }

            c = str;
            while (c != end && *c > ' ' && *c != ',' && *c != ']' && *c != '}') ++c; // TODO: discard terminating chars like ']' & '}' - focus on ranage instead
            //printf("const: [%.*s]\n", (int)(end - str), str);

            std::string_view name = {str, c};
            const value* v = get_variable != nullptr ? get_variable(name) : nullptr;
            if (v != nullptr) {
                result = *v;
            }
            else if (constants.contains(name))
                result = constants.at(name);
        }

        return result;
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


    std::string to_string() const {
        switch ((uint8_t)type) {
            case value::BOOLEAN:
                return (boolean() ? "true" : "false");
            case value::NUMBER:
                return std::to_string(number());
            case value::STRING:
                return /*'"'*/std::string{string()}/*'"'*/;
            case value::LIST:
                {
                    std::string result = "[";
                    std::span<value> s = list();
                    for (size_t i = 0; i < s.size(); ++i) {
                        result += s[i].to_string();
                        if (i != s.size() - 1) result += ", ";
                    }
                    result += ']';
                    return result;
                }
        }
        return "null";
    }

    ~value() {
        if (type == LIST) delete[] data.list.ptr;
        else if (type == STRING && data.string.copy) delete[] data.string.ptr;
    }
};

static size_t parse_options(const char* ptr, options_t& ops, std::function<const value*(std::string_view)> get_variable = nullptr, const char term = ']') {
    std::string_view option;
    const char* begin = nullptr;
    bool is_value = false;

    const char* const b = ptr;

    ++ptr;
    while (*ptr != term && *ptr != '\0') {
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
            ops[option] = value::_parse(ptr, get_variable);
            is_value = false;
            if (*ptr == term || *ptr == '\0') break;
        }
        ++ptr;
    }

    return ptr - b;
}

static void parse_options(std::string_view view, options_t& ops, std::function<const value*(std::string_view)> get_variable = nullptr) {
    std::string_view option;
    const char* begin = nullptr;
    bool is_value = false;

    for (const char* ptr = view.data(); ptr < view.end(); ++ptr) {
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
            ops[option] = value::_parse(ptr, get_variable);
            //ops[option] = value::parse({ptr, view.end()}, get_variable);
            is_value = false;
        }
    }
}

static std::string evaluate(const char* str, const char** end, std::function<const value*(std::string_view)> get_variable = nullptr) {
    const value v = value::_parse(str, get_variable);
    if (end != nullptr) *end = str;
    return v.to_string();
}

static void register_constant(std::string_view name, const value& v) {
    constants[name] = v;
}
