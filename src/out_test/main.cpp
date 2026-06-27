#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

#include "../../../serialize/serialize.hpp"

struct sec_node {
    std::string type;
    std::string title;
    std::vector<sec_node> children;
};

template <>
struct serializer<sec_node> {
    using self = sec_node;
    using fields = field_list <
        AUTO_FIELD(type),
        AUTO_FIELD(title),
        AUTO_FIELD(children)
    >;
};

int main() {
    const char* path = std::getenv("MODOC_JSON_FILE");

    if (!path) {
        std::cerr << "MY_OUTPUT_FILE not set\n";
        return 1;
    }

    std::ofstream out(path);
    if (!out) {
        std::cerr << "Failed to open " << path << '\n';
        return 1;
    }

    sec_node s {"sec", "Sekcja dodatkowa"};
    s.children.push_back({"sec", "Podsekcja 1"});
    s.children.push_back({"sec", "Podsekcja 2"});

    const std::string json = json::serialize_struct(s);

    json::pretty_print(json.c_str());
    out << json;
    
    return 0;
}
