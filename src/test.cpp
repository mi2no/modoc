#include "lsp_process.hpp"
#include "serialize/serialize.hpp"
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

    lp.open_document({uri, "cpp", content, 1});
    
    std::string tokens = lp.request_tokens("file:///tmp/main.cpp");
    puts(json::pretty(tokens.c_str()).c_str());
 
    lp.close_document(uri);
    lp.shutdown();
}
