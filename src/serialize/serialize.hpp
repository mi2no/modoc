#pragma once
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <type_traits>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <stack>
#include <functional>

template <size_t N>
struct fixed_string {
    char data[N];

    constexpr fixed_string(const char (&str)[N]) {
        for (size_t i = 0; i < N; ++i) data[i] = str[i];
    }

    constexpr operator std::string_view() const {
        return {data, N - 1};
    }
};

template <typename>
struct member_pointer_traits;

template <class C, typename T>
struct member_pointer_traits<T C::*> {
    using class_type = C;
    using value_type = T;
};

template <auto _member, /*class C, typename T, T C::*_member,*/ fixed_string _name, auto _to_serializable = nullptr, auto _from_serializable = nullptr>//const char* _name>
struct field {
    using traits = member_pointer_traits<decltype(_member)>;

    using type = traits::value_type;//T;
    using object = traits::class_type;
    //static constexpr size_t offset = _offset;
    //static constexpr T C::* member = _member;
    //static constexpr const char* name = _name;
    static constexpr type object::* member = _member;
    static constexpr auto name = _name;

    static constexpr auto to_serializable = _to_serializable;
    static constexpr auto from_serializable = _from_serializable;
};

#define AUTO_FIELD(member) \
    field<&self::member, #member>

template <typename... fields>
struct field_list {};

template <typename T>
struct serializer {};


template <typename T, typename = void>
struct is_iterable : std::false_type {};

template <typename T>
struct is_iterable<T,
    std::void_t<
        decltype(std::begin(std::declval<T>())),
        decltype(std::end(std::declval<T>()))
    >
> : std::true_type {};

template <typename T, typename = void>
struct is_serializeable : std::false_type {};

template <typename T>
struct is_serializeable<T,
    std::void_t<typename serializer<T>::fields>
> : std::true_type {};

template<typename T, typename = void>
struct has_post_deserialize : std::false_type {};

template<typename T>
struct has_post_deserialize<T,
    std::void_t<
        decltype(serializer<T>::post_deserialize(
            std::declval<T&>(),
            std::declval</*const*/ std::unordered_map<
                std::string_view,
                const char*>&>()
        ))
    >
> : std::true_type {};

template<typename T, typename = void>
struct has_from_string : std::false_type {};

template<typename T>
struct has_from_string<T,
    std::void_t<
        decltype(serializer<T>::from_string(std::declval<std::string_view>()))
    >
> : std::bool_constant<
        std::is_same_v<
            decltype(serializer<T>::from_string(std::declval<std::string_view>())),
            T
        >
    > {};

namespace json {
    template <class T, typename... fields>
    static std::string serialize_fields(/*uint8_t* src,*/ const T& a, field_list<fields...>);

    template <typename T>
    static std::string serialize_value(const T& value) {
        std::string result = "";
        if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>) {
            result += '"';

            bool esc_chars = false;
            for (char c : value) 
                if (c == '\n' || c == '\t' || c == '"') {
                    esc_chars = true;
                    break;
                }

            if (esc_chars) {
                const char* begin = value.data();
                const char* const end = value.data() + value.size();

                for (const char* ptr = begin; ptr < end; ++ptr) {
                    if (*ptr < ' ' || *ptr == '"') {
                        result += std::string_view{begin, ptr};
                       
                        if (*ptr == '\n') result += "\\n";
                        else if (*ptr == '\t') result += "\\t";
                        else if (*ptr == '"') result += "\\\"";

                        begin = ptr + 1;
                    }
                }
                if (begin < end) result += std::string_view{begin, end};
            }
            else result += value;

            result += '"';
        }
        else if constexpr (std::is_same_v<T, bool>) {
            result += value ? "true" : "false";
        }
        else if constexpr (is_iterable<T>::value) {
            result += '[';

            bool first = true;
            for (const auto& element : value) {
                if (!first) result += ',';
                first = false;

                result += serialize_value(element);
            }

            result += ']';
        }
        else if constexpr (is_serializeable<T>::value) {
            result += '{';
            result += serialize_fields(value, typename serializer<T>::fields{});
            result += '}';
        }
        else 
            result += std::to_string(value);

        return result;
    }

    template <class T, typename... fields>
    static std::string serialize_fields(/*uint8_t* src,*/ const T& a, field_list<fields...>) {
        std::string result = "";
        bool first = true;
        ((
            [&] {
                if (!first) result += ',';
                first = false;

                result += '"';
                result += fields::name;
                result += "\": ";
                if constexpr (fields::to_serializable != nullptr) {
                    const auto s_value = fields::to_serializable(a.*fields::member);
                    result += serialize_value(s_value);
                }
                else result += serialize_value(a.*fields::member);
            }()
         ), ...);
        return result;
    }

    template <typename T>
    static std::string serialize_struct(const T& a) {
        std::string result = "{";
        result += serialize_fields(/*(uint8_t*)&*/a, typename serializer<T>::fields{}); 
        return result + '}';
    }

    

    template <typename T>
    static T deserialize(const char*& json);

    template <typename T>
    static void deserialize_value(T& a, const char*& begin) {
        if constexpr (std::is_same_v<T, std::string>) {
            if (*begin == '"') {
                ++begin;

                size_t len = 0;
                bool esc = false;
                while (begin[len] != '"' || esc) {
                    if (begin[len] == '\\') esc = true;
                    else esc = false;
                    ++len;
                };
                
                a = std::string{begin, len};
                //new (&(a.*fields::member)) std::string{begin, len};
                begin += len;
            } 
        }
        else if constexpr (std::is_unsigned_v<T>) {
            char* end;
            a = (T)strtoull(begin, &end, 10);
            begin = end;
        }
        else if constexpr (std::is_integral_v<T>) {
            char* end;
            a = (T)strtoll(begin, &end, 10);
            begin = end;
        }
        else if constexpr (is_serializeable<T>::value) {
            if (*begin == '{')
                a = deserialize<T>(begin);
            else if (*begin == '"') {
                if constexpr (has_from_string<T>::value) {
                    std::string result;
                    deserialize_value(result, begin);
                    a = serializer<T>::from_string(result);
                } 
            }
        }
        else if constexpr (is_iterable<T>::value) {
            if (*begin == '[') {
                a = {};
                
                //putchar(*begin);
                //puts("");
                uint8_t depth = 1;
                while (depth) {//*begin != ']') {
                    do {
                        ++begin;
                    } while (*begin == ' ');

                    if (*begin == '[') ++depth;

                    typename T::value_type result;
                    const char* ptr = begin;
                    deserialize_value(result, ptr);

                    //printf("%c\n", *begin);

                    if (*ptr == ']') --depth;

                    if (begin != ptr) a.push_back(result);

                    begin = ptr;
                }
                ++begin;
            }                        
        }

    }

    template <class T, typename... fields>
    static void deserialize_fields(T& a, std::unordered_map<std::string_view, const char*> field_map, field_list<fields...>) {
        ((
            [&] {
                if (auto itr = field_map.find(fields::name); itr != field_map.end()) {
                    const char* begin = itr->second;

                    if constexpr (fields::from_serializable != nullptr) {
                        if constexpr (fields::to_serializable != nullptr) {
                            std::invoke_result_t<decltype(fields::to_serializable), typename fields::type> value;
                            deserialize_value(value, begin);
                            if constexpr (std::is_invocable_v<decltype(fields::from_serializable), typename fields::type, T&>)
                                a.*fields::member = fields::from_serializable(value, a);
                            else 
                                a.*fields::member = fields::from_serializable(value);
                        }
                        else {
                            typename fields::type value;
                            deserialize_value(value, begin);
                            a.*fields::member = value;
                        }
                    }
                    deserialize_value(a.*fields::member, begin);
                }                    
            }()
         ), ...);
    }

    static std::unordered_map<std::string_view, const char*> deserialize_map(const char*& json) {
        std::unordered_map<std::string_view, const char*> fields;

        while (*json != '}' && *json != '\0') {
            while (*json++ != '"');
            
            uint16_t name_len = 0;
            while (json[name_len] != '"') ++name_len;
            const std::string_view field_name = {json, name_len};
            //printf("N %.*s\n", name_len, json);
            json += name_len + 1;

            while (*json == ' ' || *json == ':') ++json; // TODO replace later with json character set (", [, t, f, num)
            fields[field_name] = json;
            //printf("V %c\n", *json);

            if (*json == '[')
                //while (*json != ']') ++json;
            {
                uint16_t depth = 1;
                ++json;
                while (depth) {
                    if (*json == '[') ++depth;
                    else if (*json == ']') --depth;
                    ++json;
                }
            }
            else if (*json == '"') {
                bool esc = false;
                ++json;
                while (*json != '"' || esc) {
                    if (*json == '\\') esc = true;
                    else esc = false;
                    ++json;
                };
            }
            else if (*json == '{') {
                uint16_t depth = 1;
                ++json;
                while (depth) {
                    if (*json == '{') ++depth;
                    else if (*json == '}') --depth;
                    ++json;
                }
            }

            while (*json != ',' && *json != '}') ++json;
        }
        //fflush(stdout);

        /*for (auto e : fields)
            printf("%.*s: %c\n", e.first.size(), e.first.data(), *e.second);*/
        
        return fields;
    }

    template <typename T>
    static T deserialize(const char*& json) {
        std::unordered_map<std::string_view, const char*> fields = deserialize_map(json);
        T result;
 
        deserialize_fields(result, fields, typename serializer<T>::fields{});

        if constexpr (has_post_deserialize<T>::value) serializer<T>::post_deserialize(result, fields);

        return result;       
    }


    template <typename T>
    static T deserialize_field(const char* json, const char* name) {
        const char* value = nullptr;

        while (*json != '}' && *json != '\0') {
            while (*json++ != '"');
           
            const char* ptr = name;
            while (*json != '"' && *ptr != '\0') {
                if (*json != *ptr) break;
                ++json;
                ++ptr;
            }

            if (*json == '"' && *ptr == '\0') { // match
                while (*++json != ':');
                while (*++json == ' ');
                value = json;
                break;
            }
            else while (*json != '"') ++json;

            ++json;

            while (*json == ' ' || *json == ':') ++json; // TODO replace later with json character set (", [, t, f, num)
            //printf("V %c\n", *json);

            if (*json == '[')
                while (*json != ']') ++json;
            else if (*json == '"') {
                bool esc = false;
                ++json;
                while (*json != '"' || esc) {
                    if (*json == '\\') esc = true;
                    else esc = false;
                    ++json;
                };
            }
            else if (*json == '{') {
                uint16_t depth = 1;
                ++json;
                while (depth) {
                    if (*json == '{') ++depth;
                    else if (*json == '}') --depth;
                    ++json;
                }
            }

            while (*json != ',' && *json != '}') ++json;
        }

        T result{};

        if (value != nullptr) deserialize_value(result, value);

        return result;
    }

    /*static uint8_t max_str_len(const char* str) {
        uint16_t max_len = 0, prev_len = 0;
        bool field_name = true;

        while (*str != '}') {
            if (*str == '{') {
                uint8_t depth = 1;
                while (depth > 0)
                    if (*str == '{') ++depth;
                    else if (*str == '}') --depth;
            }
            else if (*str == '"') {
                prev_len = 1;
                while (str[prev_len++] != '"');
                str += prev_len - 1;
            }
            else if (*str == ':') {
                max_len = std::max(max_len, prev_len);
            }

            ++str;
        }

        return prev_len;
    }*/

    static void pretty_print(const char* json) {
        bool indent = false;
        uint8_t object_depth = 0;

        enum : uint8_t {
            LIST, OBJ
        };
        std::stack<uint8_t> in;
        
        while (*json != '\0') { 
            if (*json == '{') {
                ++object_depth;
                in.push(OBJ);
                //puts("\033[1m{\033[0m");
                puts("{");
                indent = true;
            }
            else if (*json == '}') {
                --object_depth;
                in.pop();

                putchar('\n');
                for (uint8_t i = 0; i < object_depth; ++i) fputs("    ", stdout);
                //fputs("\033[1m}\033[0m", stdout);
                putchar('}');

                indent = false; //
            }
            else if (*json == ':') {
                fputs(" : ", stdout);
            }
            else if (*json == '"') {
                uint16_t len = 1;
                bool esc = false;
                while (json[len] != '"' || esc) {
                    if (json[len] == '\\') esc = true;
                    else esc = false;
                    ++len;
                };
                ++len;

                if (indent) printf("\033[38;5;141m%.*s\033[0m", len, json);
                else printf("\033[38;5;229m%.*s\033[0m", len, json);

                //printf("\033[32m%.*s\033[0m", len, json);
                json += len - 1;

                indent = false; //
            }
            else if (*json == '-' || (*json >= '0' && *json <= '9') || *json == '.') {
                uint16_t len = 1;
                while (json[len] != ',' && json[len] != ' ' && json[len] != '\n' && json[len] != ']' && json[len] != '}') ++len;

                printf("\033[38;5;6m%.*s\033[0m", len, json);
                //printf("\033[33m%.*s\033[0m", len, json);
                json += len - 1;
            }
            else if (*json == 't') { // true
                //fputs("\033[34mtrue\033[0m", stdout);
                fputs("\033[38;5;216mtrue\033[0m", stdout);
                json += 3;
            }
            else if (*json == 'f') { // false
                //fputs("\033[34mfalse\033[0m", stdout);
                fputs("\033[38;5;216mfalse\033[0m", stdout);
                json += 4;
            }
            else if (*json == 'n') { // null
                fputs("\033[38;5;60mnull\033[0m", stdout);
                json += 3;
            }
            else if (*json == '[') {
                putchar('[');
                in.push(LIST);
            }
            else if (*json == ']') {
                putchar(']');
                in.pop();
            }
            else if (*json == ',') {
                putchar(',');
                if (in.top() == OBJ) {
                    putchar('\n');
                    indent = true;
                }
                else putchar(' ');
            }

            if (indent) {
                for (uint8_t i = 0; i < object_depth; ++i) fputs("    ", stdout);
                //indent = false;
            }

            ++json;
        }
        putchar('\n');
    }


    static std::string pretty(const char* json) {
        bool indent = false;
        uint8_t object_depth = 0;

        enum : uint8_t {
            LIST, OBJ
        };
        std::stack<uint8_t> in;

        std::string result;
        
        while (*json != '\0') { 
            if (*json == '{') {
                ++object_depth;
                in.push(OBJ);
                //puts("\033[1m{\033[0m");
                result += "{\n";
                indent = true;
            }
            else if (*json == '}') {
                --object_depth;
                in.pop();

                result += '\n';
                for (uint8_t i = 0; i < object_depth; ++i) result += "    ";
                //fputs("\033[1m}\033[0m", stdout);
                result += '}';

                indent = false; //
            }
            else if (*json == ':') {
                result += " : ";
            }
            else if (*json == '"') {
                uint16_t len = 1;
                bool esc = false;
                while (json[len] != '"' || esc) {
                    if (json[len] == '\\') esc = true;
                    else esc = false;
                    ++len;
                };
                ++len;

                if (indent) result += "\033[38;5;141m";
                else result += "\033[38;5;229m";

                result += {json, json + len};
                result += "\033[0m";

                //printf("\033[32m%.*s\033[0m", len, json);
                json += len - 1;

                indent = false; //
            }
            else if (*json == '-' || (*json >= '0' && *json <= '9') || *json == '.') {
                uint16_t len = 1;
                while (json[len] != ',' && json[len] != ' ' && json[len] != '\n' && json[len] != ']' && json[len] != '}') ++len;

                result += "\033[38;5;6m";
                result += {json, json + len};
                result += "\033[0m";
                //printf("\033[33m%.*s\033[0m", len, json);
                json += len - 1;
            }
            else if (*json == 't') { // true
                //fputs("\033[34mtrue\033[0m", stdout);
                result += "\033[38;5;216mtrue\033[0m";
                json += 3;
            }
            else if (*json == 'f') { // false
                //fputs("\033[34mfalse\033[0m", stdout);
                result += "\033[38;5;216mfalse\033[0m";
                json += 4;
            }
            else if (*json == 'n') { // null
                result += "\033[38;5;60mnull\033[0m";
                json += 3;
            }
            else if (*json == '[') {
                result += '[';
                in.push(LIST);
            }
            else if (*json == ']') {
                result += ']';
                in.pop();
            }
            else if (*json == ',') {
                result += ',';
                if (in.top() == OBJ) {
                    result += '\n';
                    indent = true;
                }
                else result += ' ';
            }

            if (indent) {
                for (uint8_t i = 0; i < object_depth; ++i) result += "    ";
                //indent = false;
            }

            ++json;
        }
        return result;
    }
}
