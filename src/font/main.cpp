#include "ttf.hpp"

#include <bit>
#include <cstdio>

int main() {
    constexpr const char* path = "JetBrainsMono/static/JetBrainsMono-Regular.ttf";
    
    load_ttf(path);
}
