#pragma once

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ios>
#include <span>
#include <iostream>

#include "endian.hpp"

using fixed_t = uint32_t;
using long_date_time_t = int64_t;
using version_16_dot_16_t = uint32_t;

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
    CMAP = tag("cmap"),
    HEAD = tag("head"),
    MAXP = tag("maxp"),
    GLYF = tag("glyf"),
    LOCA = tag("loca")
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

/* ======================================================
 * HEAD
 * ======================================================
 */

struct head_t {
    uint16_t major_version;
    uint16_t minor_version;
    fixed_t font;
    uint32_t checksum_adjustment;
    uint32_t magic_number;
    uint16_t flags;
    uint16_t units_per_em;
    long_date_time_t created;
    long_date_time_t modified;
    int16_t x_min;
    int16_t y_min;
    int16_t x_max;
    int16_t y_max;
    uint16_t mac_style;
    uint16_t lowest_rec_ppem;
    int16_t font_direction_hint;
    int16_t index_to_loc_format;
    int16_t glyph_data_format;
};

template <>
struct layout<head_t> {
    using groups = group_list<
        group<std::endian::big, &head_t::major_version, &head_t::minor_version, &head_t::font, &head_t::checksum_adjustment, &head_t::magic_number, &head_t::flags, &head_t::units_per_em, &head_t::created, &head_t::modified, &head_t::x_min, &head_t::y_min, &head_t::x_max, &head_t::y_max, &head_t::mac_style, &head_t::lowest_rec_ppem, &head_t::font_direction_hint, &head_t::index_to_loc_format, &head_t::glyph_data_format>
    >;
};

static void head(std::span<uint8_t> buffer) {
    const uint8_t* ptr = buffer.data();

    head_t head = read_cast<head_t>(ptr);
    ptr += packed_size<head_t>();

    printf("MajorVersion: %hu\nMinorVersion: %hu\nMagic: %u\n", head.major_version, head.minor_version, head.magic_number);
}

/* ======================================================
 * MAXP
 * ======================================================
 */

struct maxp_t {
    version_16_dot_16_t version;
    uint16_t num_glyphs;
    uint16_t max_points;
    uint16_t max_contours;
    uint16_t max_composite_points;
    uint16_t max_composite_contours;
    uint16_t max_zones;
    uint16_t max_twilight_points;
    uint16_t max_storage;
    uint16_t max_function_defs;
    uint16_t max_instruction_defs;
    uint16_t max_stack_elements;
    uint16_t max_size_of_instructions;
    uint16_t max_component_elements;
    uint16_t max_component_depth;

    static maxp_t get(std::span<uint8_t> buffer) {
        const uint8_t* ptr = buffer.data();

        maxp_t result = {read_cast_simple<version_16_dot_16_t, std::endian::big>(ptr), read_cast_simple<uint16_t, std::endian::big>(ptr + sizeof(version_16_dot_16_t))};

        if (result.version == 0x00010000) read_cast(result, ptr + sizeof(version_16_dot_16_t) + sizeof(uint16_t));

        std::cout << "MAXP\nversion : " << std::hex << result.version << std::dec << "\nnum glyphs : " << result.num_glyphs << '\n';

        return result;
    }
};

template <>
struct layout<maxp_t> {
    using groups = group_list<
        group<std::endian::big, &maxp_t::max_points, &maxp_t::max_contours, &maxp_t::max_composite_points, &maxp_t::max_composite_contours, &maxp_t::max_zones, &maxp_t::max_twilight_points, &maxp_t::max_storage, &maxp_t::max_function_defs, &maxp_t::max_instruction_defs, &maxp_t::max_stack_elements, &maxp_t::max_size_of_instructions, &maxp_t::max_component_elements, &maxp_t::max_component_depth>
    >;
};


/* ======================================================
 * LOCA
 * ======================================================
 */

static inline uint32_t* loca(std::span<uint8_t> buffer, uint16_t num_glyphs, int16_t index_to_loc_format) {
    uint32_t* result = nullptr;
    const uint32_t count = num_glyphs + 1;

    if (index_to_loc_format == 0) { // Short offset
        result = new uint32_t[count];
        for (uint32_t i = 0; i < count; ++i) result[i] = read_cast_simple<uint16_t, std::endian::big>(buffer.data() + i * sizeof(uint16_t)) << 1; 
    }
    else if (index_to_loc_format == 1) { // Long offset
        result = new uint32_t[count];
        memcpy(result, buffer.data(), count * sizeof(uint32_t));

        for (uint32_t i = 0; i < count; ++i) result[i] = cast_to_native<std::endian::big>(result[i]);
    }

    return result;
}

/* =======================================================
 * GLYF
 * =======================================================
 */

struct glyph_t {
    int16_t number_of_contours;
    int16_t x_min;
    int16_t y_min;
    int16_t x_max;
    int16_t y_max;

    static glyph_t* get(std::span<uint8_t> buffer, std::span<uint32_t> loca);
};

template <>
struct layout<glyph_t> {
    using groups = group_list<
        group<std::endian::big, &glyph_t::number_of_contours, &glyph_t::x_min, &glyph_t::y_min, &glyph_t::x_max, &glyph_t::y_max>
    >;
};

glyph_t* glyph_t::get(std::span<uint8_t> buffer, std::span<uint32_t> loca) {
    puts("GLYF");
    for (size_t i = 0; i < loca.size() - 1; ++i) {
        const uint8_t * ptr = buffer.data() + loca[i], * const end = buffer.data() + loca[i + 1];

        glyph_t g = read_cast<glyph_t>(ptr);
        ptr += packed_size<glyph_t>();

        std::cout << "num: " << g.number_of_contours << '\n';
        if (g.number_of_contours >= 0) { // Simple
            uint16_t* end_pts_of_contours = read_cast_simple<uint16_t, std::endian::big>(ptr, g.number_of_contours);
            ptr += sizeof(uint16_t) * g.number_of_contours;

            uint16_t instruction_length = read_cast_simple<uint16_t, std::endian::big>(ptr);
            ptr += sizeof(uint16_t);

            uint8_t* instructions = read_cast_simple<uint8_t, std::endian::big>(ptr, instruction_length);

            delete[] end_pts_of_contours;
            delete[] instructions;
        }
        else { // Composite

        }
        fflush(stdout);
    }
    return nullptr;
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

struct ttf_resource {
    head_t head;
    cmap_t cmap;
    maxp_t maxp;
};

static ttf_resource load_ttf(const char* path) {
    size_t size;
    uint8_t* buffer = readf(path, size);

    printf("%zu\n", size);

    table_t table = read_cast<table_t>(buffer); 

    printf("%X\n%hu\n", table.sfnt_version, table.table_count);

    uint8_t* ptr = buffer + packed_size<table_t>();

    ttf_resource ttf;
    uint32_t glyf_off = 0, loca_off = 0;
    uint32_t* loca_arr = nullptr;

    for (size_t i = 0; i < table.table_count; ++i) {
        table_record_t record = read_cast<table_record_t>(ptr);

        std::span<uint8_t> span = {buffer + record.offset, buffer + size};
        switch (record.tag) {
            case CMAP:
                /*ttf.cmap =*/ cmap(span);
                break;
            case HEAD:
                head(span);
                break;
            case MAXP:
                ttf.maxp = maxp_t::get(span);
                break;
            case GLYF:
                glyf_off = record.offset;
                break;
            case LOCA:
                loca_off = record.offset;
                break;
        }

        print_tag(record.tag);
        printf(" %10u %10u\n", record.offset, record.length);

        ptr += packed_size<table_record_t>();
    } 

    if (loca_off) {
        loca_arr = loca({buffer + loca_off, buffer + size}, ttf.maxp.num_glyphs, ttf.head.index_to_loc_format);
        puts("LOCA");
        for (size_t i = 0; i <= ttf.maxp.num_glyphs; ++i) std::cout << loca_arr[i] << '\n';
    }
    if (glyf_off && loca_arr != nullptr) {
        glyph_t::get({buffer + glyf_off, buffer + size}, {loca_arr, loca_arr + ttf.maxp.num_glyphs + 1});
    }

    if (loca_arr != nullptr) delete[] loca_arr;
    if (buffer != nullptr) free(buffer);

    return ttf;
}
