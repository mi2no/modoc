#include "ttf.hpp"
#include <memory>

int main() {
    constexpr const char* path = "JetBrainsMono/static/JetBrainsMono-Regular.ttf";
    
    load_ttf(path);

    std::unique_ptr<uint8_t[]> ptr = std::make_unique<uint8_t[]>(10);

    std::cout << sizeof(ptr) << '\n';
}
