#include "ttf.hpp"
#include <memory>

int main(int argc, char** argv) {
    constexpr const char* jbm_path = "JetBrainsMono/static/JetBrainsMono-Regular.ttf";

    const char* path = (argc >= 2) ? argv[1] : jbm_path;
    
    load_ttf(path);
}
