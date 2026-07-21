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

        [[deprecated("Prefer std::move()")]]
        string_type(const string_type& s) {
            owned = s.owned;
            size = s.size;

            if (owned) {
                char* temp = new char[size];
                memcpy(temp, s.ptr, size);
                ptr = temp;
            }
            else ptr = s.ptr;
        }

        string_type(string_type&& s) noexcept {
            ptr = s.ptr;
            size = s.size;
            owned = s.owned;
            s.owned = false;
        }

        [[deprecated("Prefer std::move()")]]
        string_type& operator=(const string_type& s) {
            if (this == &s) return *this;
            if (owned) delete[] ptr;

            owned = s.owned;
            size = s.size;

            if (owned) {
                char* temp = new char[size];
                memcpy(temp, s.ptr, size);
                ptr = temp;
            }
            else ptr = s.ptr;

            return *this;
        }

        string_type& operator=(string_type&& s) noexcept {
            if (this == &s) return *this;
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

        void remove_prefix(size_t n) {
            if (owned) {
                if (n >= size) {
                    delete[] ptr;
                    size = 0;
                    ptr = nullptr;
                    owned = false;
                }
                else {
                    size -= n;
                    char* const temp = new char[size];
                    memcpy(temp, ptr + n, size);
                    
                    delete[] ptr;
                    ptr = temp;
                }
            }
            else {
                if (n >= size) {
                    size = 0;
                    ptr = nullptr;
                }
                else {
                    size -= n;
                    ptr += n;
                }
            }
        }

        ~string_type() {
            if (owned) delete[] ptr;
        }
    };
};
