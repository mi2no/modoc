#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

#include <iostream>

#include "../../../teams_cli/serialize.hpp"
#include "log.hpp"

namespace msg {
    constexpr const char* header = "Content-Length:";
    constexpr size_t header_len = std::char_traits<char>::length(header);

    static void send(std::string_view json) {
        log(json);
        printf("Content-Length: %zu\r\n\r\n%s", json.size(), json.data());
        fflush(stdout);


    }

    static std::string read() {
        std::string line;
        size_t content_len = 0;

        while (std::getline(std::cin, line)) {
            if (line.empty() || line == "\r") break;

            if (strncmp(line.c_str(), header, header_len) == 0)
                content_len = strtoull(line.c_str() + header_len + 1, nullptr, 10);
        }

        if (!content_len) return "";

        std::string body(content_len, '\0');
        std::cin.read(body.data(), content_len);

        return body;
    }

    struct method_t {
        uint8_t id;
        std::string method;
    };
};

template <>
struct serializer<msg::method_t> {
    using self = msg::method_t;

    using fields = field_list<
        AUTO_FIELD(id),
        AUTO_FIELD(method)
    >;
};
