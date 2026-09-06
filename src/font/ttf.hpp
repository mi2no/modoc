#pragma once

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <span>

#include "endian.hpp"

consteval uint32_t tag(const char (&ptr)[5]) {
    return  ((uint32_t)ptr[0] << 24) |
            ((uint32_t)ptr[1] << 16) |
            ((uint32_t)ptr[2] << 8)  |
            ((uint32_t)ptr[3]);
}

inline void print_tag(uint32_t tag) {
    putchar((tag >> 24) & 0xFF);
    putchar((tag >> 16) & 0xFF);
    putchar((tag >> 8) & 0xFF);
    putchar(tag & 0xFF);
}

enum : uint32_t {
    CMAP = tag("cmap")
};

uint8_t* readf(const char* path, size_t& size) {
    FILE* file = fopen(path, "rb");

    if (file == nullptr) return nullptr;

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    rewind(file);

    uint8_t* buffer = (uint8_t*)malloc(size);
    fread(buffer, 1, size, file);
    fclose(file);

    return buffer;
}

/* =======================================================
 * CMAP
 * =======================================================
 */

struct cmap_t {
    uint16_t version;
    uint16_t table_count;
};

struct encoding_record_t {
    uint16_t platform_id;
    uint16_t encoding_id;
    uint32_t offset;
};

struct format_4_t {
    uint16_t length;
    uint16_t lang;
    uint16_t seg_count;
    uint16_t search_range;
    uint16_t entry_selector;
    uint16_t range_shift;

    uint16_t* end_code;
    uint16_t* start_code;
    uint16_t* id_delta;
    uint16_t* id_range_offset;
    uint16_t* glyph_id_array;

    void print() {
        printf("length: %hu\nlang: %hu\nseg_count: %hu\nsearch_range: %hu\nentry_selector: %hu\nrange_shift: %hu\n", length, lang, seg_count, search_range, entry_selector, range_shift);
    }
};

template <>
struct layout<cmap_t> {
    using groups = group_list<
        group<std::endian::big, &cmap_t::version, &cmap_t::table_count>
    >;
};

template <>
struct layout<encoding_record_t> {
    using groups = group_list<
        group<std::endian::big, &encoding_record_t::platform_id, &encoding_record_t::encoding_id, &encoding_record_t::offset>
    >;
};

template <>
struct layout<format_4_t> {
    using groups = group_list<
        group<std::endian::big, &format_4_t::length, &format_4_t::lang, &format_4_t::seg_count, &format_4_t::search_range, &format_4_t::entry_selector, &format_4_t::range_shift>
    >;
};

void cmap(std::span<uint8_t> buffer) {
    const uint8_t* ptr = buffer.data();

    cmap_t cmap = read_cast<cmap_t>(ptr);
    ptr += packed_size<cmap_t>();

    printf("cmap:\n%hu %hu\n", cmap.version, cmap.table_count);

    for (size_t i = 0; i < cmap.table_count; ++i) {
        encoding_record_t er = read_cast<encoding_record_t>(ptr);
        
        const uint8_t* subtable = buffer.data() + er.offset;
        const uint16_t format_id = cast_to_native<std::endian::big>(*(uint16_t*)subtable);

        printf("=====\nPlatform id: %hu\nEncoding id: %hu\nOffset: %u\nFormat id: %hu\n=====\n", er.platform_id, er.encoding_id, er.offset, format_id);
        
        switch (format_id) {
            case 4:
            {
                format_4_t tbl = read_cast<format_4_t>(subtable + sizeof(format_id));
                tbl.print();

                tbl.end_code = (uint16_t*)(subtable + sizeof(format_id) + sizeof(uint16_t) * 6);
                for (size_t i = 0; i < tbl.seg_count; ++i)
                    printf("%hu\n", cast_to_native<std::endian::big>(tbl.end_code[i]));
            }
        }

        ptr += packed_size<encoding_record_t>();
    }
}

/* =======================================================
 * LOAD_TTF
 * =======================================================
 */

struct table_t {
    uint32_t sfnt_version;
    uint16_t table_count;
    uint16_t search_range;
    uint16_t entry_selector;
    uint16_t range_shift;
};

struct table_record_t {
    uint32_t tag; // 4 char array
    uint32_t checksum;
    uint32_t offset;
    uint32_t length;
}; 

template <>
struct layout<table_t> {
    using groups = group_list<
        group<std::endian::big, &table_t::sfnt_version, &table_t::table_count, &table_t::search_range, &table_t::entry_selector, &table_t::range_shift>
    >;
};

template <>
struct layout<table_record_t> {
    using groups = group_list<
        group<std::endian::big, &table_record_t::tag, &table_record_t::checksum, &table_record_t::offset, &table_record_t::length>
    >;
};

static void load_ttf(const char* path) {
    size_t size;
    uint8_t* buffer = readf(path, size);

    printf("%zu\n", size);

    table_t table = read_cast<table_t>(buffer); 

    printf("%X\n%hu\n", table.sfnt_version, table.table_count);

    uint8_t* ptr = buffer + packed_size<table_t>();

    uint32_t cmap_off = 0;

    for (size_t i = 0; i < table.table_count; ++i) {
        table_record_t record = read_cast<table_record_t>(ptr);

        if (record.tag == CMAP) cmap_off = record.offset;

        print_tag(record.tag);
        printf(" %10u %10u\n", record.offset, record.length);

        ptr += packed_size<table_record_t>();
    } 

    cmap({buffer + cmap_off, buffer + size});

    if (buffer != nullptr) free(buffer);
}
