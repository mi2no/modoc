#pragma once

#include <cstdint>
#include <cstring>
#include <utility>
#include <cstdlib>
#include <bit>

template <typename>
struct member_pointer_traits;

template <class C, typename T>
struct member_pointer_traits<T C::*> {
    using class_type = C;
    using value_type = T;
};

template <typename T, size_t... i>
void swap_endian(void* vptr, std::index_sequence<i...>) {
    T* ptr = (T*)vptr;
    ((ptr[i] = [] (T v) -> T {
        if constexpr (sizeof(T) == sizeof(uint64_t))
            return __builtin_bswap64(v);
        else if constexpr (sizeof(T) == sizeof(uint32_t))
            return __builtin_bswap32(v); 
        else if constexpr (sizeof(T) == sizeof(uint16_t))
            return __builtin_bswap16(v);
        else
            return v;
    }(ptr[i])), ...);
}

template <std::endian _endian, auto... members>
struct group {
    static constexpr std::endian endian = _endian;

    template <typename T>
    static void apply(T& dest, const uint8_t*& src) {
        (
            [&] {
                using member_type = typename member_pointer_traits<decltype(members)>::value_type;

                memcpy(&(dest.*members), src, sizeof(member_type));

                if constexpr (endian != std::endian::native) {
                    swap_endian<member_type>(&(dest.*members), std::make_index_sequence<1>{});
                }

                src += sizeof(member_type);
            }(),
            ...
        );
    }
};

template <typename T>
struct layout;

template <typename... groups>
struct group_list {};

/*template <std::endian from, std::endian to, typename... groups>
void apply_groups(uint8_t* dest, uint8_t* src, group_list<groups...>) {
    uint8_t* dest_i;
    if constexpr (from != to) {
        ((
            dest_i = dest + groups::offset,
            memcpy(dest_i, src, groups::byte_size),
            swap_endian<typename groups::type>(dest_i, std::make_index_sequence<groups::count>{}),
            src += groups::byte_size
         ), ...);
    }
    else {
        ((
            dest_i = dest + groups::offset,
            memcpy(dest_i, src, groups::byte_size),
            src += groups::byte_size
         ), ...);
    }
}*/

template <std::endian target, typename T, typename... groups>
void apply_groups(T& dest, const uint8_t* src, group_list<groups...>) {
    uint8_t* dest_i;
    ((
        //dest_i = dest + groups::offset,
        /*memcpy(&(dest.*groups::member), src, groups::size),
        [&] {
            if constexpr (groups::endian != target)
                swap_endian<typename groups::type>(&(dest.*groups::member), std::make_index_sequence<groups::count>{});
        }(),
        src += groups::size*/
        groups::apply(dest, src)
     ), ...);
}


/*template <std::endian from, std::endian to, typename T>
void read_cast(T& a, uint8_t* buffer) {
    apply_groups<from, to>((uint8_t*)&a, buffer, typename layout<T>::groups{});
}*/

template <typename T, std::endian target = std::endian::native>
T read_cast(const uint8_t* buffer) {
    T a{};
    apply_groups<target>(a, buffer, typename layout<T>::groups{});
    return a;
}

template <std::endian from, std::endian to, typename T>
T cast_endian(T a) {
    if constexpr (from != to) {
        if constexpr (sizeof(T) == sizeof(uint64_t))
            return __builtin_bswap64(a);
        else if constexpr (sizeof(T) == sizeof(uint32_t))
            return __builtin_bswap32(a); 
        else if constexpr (sizeof(T) == sizeof(uint16_t))
            return __builtin_bswap16(a);
        else
            return a;
    }
    else return a;
}

template <std::endian from, typename T>
T cast_to_native(T a) {
    return cast_endian<from, std::endian::native>(a);
}




template <typename T>
consteval size_t packed_size() {
    return []<typename... groups>(group_list<groups...>) {
        return ((
            []<auto... members>(group<groups::endian, members...>) {
                return (sizeof(
                    typename member_pointer_traits<decltype(members)>::value_type
                ) + ...);
            }(groups{})
        ) + ...);
    }(typename layout<T>::groups{});
}
