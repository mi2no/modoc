#pragma once
#include <string_view>
#include <iostream>
#include <map>

struct object {
    size_t _size = 1;
    union {
        std::string_view* ptr = nullptr;
        std::string_view view;
    };

    object() = default;

    object(const std::string_view& value) {
        for (const char& c : value) if (c == ',') ++_size;
        if (_size == 1) view = value;
        else {
            ptr = (std::string_view*)malloc(sizeof(std::string_view) * _size);
            size_t i = 0, begin = 0, id = 0;
            bool word = false;
            while (i < value.size()) {
                if (value[i] > ' ' && value[i] != ',' && !word) {
                   begin = i;
                   word = true;
                }
                else if ((value[i] <= ' ' || value[i] == ',') && word) {
                    new (ptr + id++) std::string_view{value.begin() + begin, i - begin};
                    word = false;
                }
                ++i;
            }
            if (word) new (ptr + id) std::string_view{value.begin() + begin, i - begin};
        }
    }

    object(object&& o) {
        _size = o._size;
        if (_size > 1) {
            ptr = o.ptr;
            o.ptr = nullptr;
        }
        else view = o.view;
        o._size = 0;
    }

    object& operator=(object&& o) {
        _size = o._size;
        if (_size > 1) {
            ptr = o.ptr;
            o.ptr = nullptr;
        }
        else view = o.view;
        o._size = 0;
        return *this;
    }

    /*object& operator==(const object& o) {
        _size = o._size;
        if (_size > 1) {
            ptr = (std::string_view*)malloc(sizeof(std::string_view) * _size);
            for (size_t i = 0; i < _size; ++i) new (ptr + i) std::string_view(o.ptr[i]);
        }
        else view = o.view;
        return *this;
    }*/



    void out() const {
        if (_size == 1) std::cout << view;
        else {
            fputs("list[", stdout);
            std::cout << *ptr;
            for (size_t i = 1; i < _size; ++i) std::cout << ", " << ptr[i];
            putchar(']');
        }
    }

    bool is_list() const {
        return _size > 1;
    }

    const size_t& size() const {
        return _size;
    }

    ~object() {
        if (_size > 1) {
            //while (_size--) ptr[_size].~std::basic_string_view<char>();
            free(ptr);
        }
    }
};

typedef std::map<std::string_view, object> options_t;

//std::map<std::string_view, object> map;

size_t get_options(const char* const& str, options_t& map) {
    bool word = false, is_value = false;
    size_t begin = 0, i = 0;
    std::string_view option;

    while (str[i] != ']' && str[i] != '\0') {
        if (str[i] == '=') {
            if (word) option = {str + begin, i - begin};
            is_value = true;
        }
        else if (str[i] == '[') {
            begin = i + 1;
            while (str[i] != ']') ++i;
            if (is_value) {
                std::string_view value{str + begin, i - begin};
                map[option] = value;
                is_value = false;
            }
        }
        else if (str[i] > ' ' && !word) {
            begin = i;
            word = true;
        }
        else if ((str[i] <= ' ' || str[i] == ',') && word) {
            if (is_value) {
                std::string_view value{str + begin, i - begin};
                map[option] = value;
                is_value = false;
            }
            else {
                option = {str + begin, i - begin};
                //std::cout << option << '\n';
            }
            word = false;
        }
        ++i;
    }

    if (is_value) {
        std::string_view value{str + begin, i - begin};
        map[option] = value;
        is_value = false;
    }

    return i;
}



/*int main() {
    constexpr const char* str = "x = 10, y = 20, c = [1, 3, 4] theme = frappe";

    bool word = false, is_value = false;
    size_t begin = 0;
    std::string_view option;
    size_t i = 0;
    while(str[i] != '\0') {
        if (str[i] == '=') {
            if (word) option = {str + begin, i - begin};
            is_value = true;
        }
        else if (str[i] == '[') {
            begin = i + 1;
            while (str[i] != ']') ++i;
            if (is_value) {
                std::string_view value{str + begin, i - begin};
                map[option] = value;
                is_value = false;
            }
        }
        else if (str[i] > ' ' && !word) {
            begin = i;
            word = true;
        }
        else if ((str[i] <= ' ' || str[i] == ',') && word) {
            if (is_value) {
                std::string_view value{str + begin, i - begin};
                map[option] = value;
                is_value = false;
            }
            else {
                option = {str + begin, i - begin};
                std::cout << option << '\n';
            }
            word = false;
        }
        ++i;
    }
    if (is_value) {
        std::string_view value{str + begin, i - begin};
        map[option] = value;
        is_value = false;
    }
            

    for (auto& a : map) {
        std::cout << a.first << " : ";
        a.second.out();
        std::cout << '\n';
    }
}*/
