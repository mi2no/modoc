#pragma once

#include <cstdint>
#include <filesystem>

#include "../../serialize/serialize.hpp"
#include "../../util.hpp"
#include "../../value.hpp"

namespace modoc {

    struct theme {
        uint32_t background;
        uint32_t background1;
        uint32_t background2;
        uint32_t surface1;
        uint32_t surface2;
        uint32_t surface3;
        uint32_t text;
        uint32_t text1;
        uint32_t text2;
        uint32_t text3;
        uint32_t text4;
        uint32_t text5;
        uint32_t color1;
        uint32_t color2;
        uint32_t color3;
        uint32_t color4;
        uint32_t color5;
        uint32_t color6;
        uint32_t color7;
        uint32_t color8;
        uint32_t color9;
        uint32_t color10;
        uint32_t color11;
        uint32_t color12;   
        uint32_t color13;  
        uint32_t color14;

    private:

        template <typename... fields>
        static value to_value_helper(const theme& t, field_list<fields...>) {
            value::object_t map;
            ((
                map[std::string((std::string_view)fields::name)] = t.*fields::member
             ), ...);
            return value::from_object(std::move(map));
        }

    public:

        value to_value() const;

        static void init() {
            std::filesystem::directory_iterator itr("objects/themes");
            value::object_t theme_obj;

            for (auto entry : itr) {
                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                    const std::span<uint8_t> buffer = modoc::read_file(entry.path().string());

                    const char* ptr = (const char*)buffer.data();
                    auto map = json::deserialize_map(ptr);

                    value::object_t obj;
                    for (auto map_entry : map) {
                        ptr = map_entry.second;
                        theme t = json::deserialize<theme>(ptr);

                        obj[std::string(map_entry.first)] = std::move(t.to_value());
                    }
                    theme_obj[entry.path().stem().string()] = value::from_object(std::move(obj));

                    delete[] buffer.data();
                }
            }

            register_constant("theme", value::from_object(std::move(theme_obj)));
        }  
    };
}

template <>
struct serializer<modoc::theme> {
    using self = modoc::theme;
    using fields = field_list<
        AUTO_FIELD(background),
        AUTO_FIELD(background1),
        AUTO_FIELD(background2),
        AUTO_FIELD(surface1),
        AUTO_FIELD(surface2),
        AUTO_FIELD(surface3),
        AUTO_FIELD(text),
        AUTO_FIELD(text1),
        AUTO_FIELD(text2),
        AUTO_FIELD(text3),
        AUTO_FIELD(text4),
        AUTO_FIELD(text5),
        AUTO_FIELD(color1),
        AUTO_FIELD(color2),
        AUTO_FIELD(color3),
        AUTO_FIELD(color4),
        AUTO_FIELD(color5),
        AUTO_FIELD(color6),
        AUTO_FIELD(color7),
        AUTO_FIELD(color8),
        AUTO_FIELD(color9),
        AUTO_FIELD(color10),
        AUTO_FIELD(color11),
        AUTO_FIELD(color12),
        AUTO_FIELD(color13),
        AUTO_FIELD(color14)
    >;
};

value modoc::theme::to_value() const {
    return to_value_helper(*this, typename serializer<theme>::fields{}); 
}


