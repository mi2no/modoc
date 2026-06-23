#pragma once
#include <string>

#include "../../../teams_cli/serialize.hpp"

struct text_document {
    std::string version;
    std::string text;
    std::string language_id;
    std::string uri;
};

template <>
struct serializer<text_document> {
    using self = text_document;

    using fields = field_list<
        AUTO_FIELD(version),
        AUTO_FIELD(text),
        field<&self::language_id, "languageId">,
        AUTO_FIELD(uri)
    >;
};

struct position {
    uint32_t line;
    uint32_t character;
};

template <>
struct serializer<position> {
    using self = position;

    using fields = field_list<
        AUTO_FIELD(line),
        AUTO_FIELD(character)
    >;
};

struct params {
    text_document text_doc;
    position pos;
};

template <>
struct serializer<params> {
    using self = params;

    using fields = field_list<
        field<&params::pos, "position">,
        field<&params::text_doc, "textDocument">
    >;
};
