#include "lsp_process.hpp"

int main() {
    modoc::lsp_process lp{"clangd"};

    lp.init();
    lp.shutdown();
}
