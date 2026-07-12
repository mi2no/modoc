#pragma once
#include <cstddef>
#include <cstring>
#include <string_view>

namespace modoc {
    class string_type {
        const char* ptr;
        size_t size;
        bool owned;

    public:

        string_type() {
            ptr = nullptr;
            size = 0;
            owned = false;
        }

        string_type(std::string_view view, bool own) : size(view.size()), owned(own) {
            if (own) {
                char* temp = new char[size];
                memcpy(temp, view.data(), size);
                ptr = temp;
            }
            else {
                ptr = view.data();
            }
        }

        string_type(string_type&& s) {
            ptr = s.ptr;
            size = s.size;
            owned = s.owned;
            s.owned = false;
        }

        string_type& operator=(string_type&& s) {
            if (owned) delete[] ptr;

            ptr = s.ptr;
            size = s.size;
            owned = s.owned;
            s.owned = false;

            return *this;
        }

        std::string_view view() const {
            return {ptr, ptr + size};
        }

        bool is_owned() const {
            return owned;
        }

        ~string_type() {
            if (owned) delete[] ptr;
        }
    };
};
