#include "lsp_process.hpp"
#include "serialize/serialize.hpp"
#include <bit>
#include <string_view>

int main() {
    constexpr std::string_view content = 
    "int main() {\n\
\tconstexpr uint16_t size = 200;\n\
\t\tdouble x[size]{};\n\
\n\
\tfor (uint16_t i = 0; i < size; ++i) x[i] = i;\n\
\n\
\treturn 0;\n\
}";

    constexpr std::string_view uri = "file:///tmp/main.cpp";

    modoc::lsp_process lp;

    
    lp.init("clangd");

    printf("types: %s\n", json::serialize_value(lp.token_types).c_str());
    printf("modifiers: %s\n", json::serialize_value(lp.token_modifiers).c_str());

    lp.open_document({uri, "cpp", content, 1});
    
    std::vector<modoc::lsp_process::token> tokens = lp.request_tokens("file:///tmp/main.cpp");
    for (const auto& t : tokens) {
        std::cout << t.str << ' ' << lp.token_types[t.type_id] << " |";

        uint32_t mods = t.modifiers;
        uint16_t id = 0;
        while (mods) {
            const uint16_t off = std::countr_zero(mods);
            std::cout << ' ' << lp.token_modifiers[id += off];
            
            mods >>= (off + 1);
            ++id;
        }
        putchar('\n');
    }

    lp.close_document(uri);
    lp.shutdown();
}
