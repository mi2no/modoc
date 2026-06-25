#include <cstdlib>
#include <fstream>
#include <iostream>

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

    out << "{\"type\":\"sec\",\"title\":\"Sekcja dodatkowa\"}\n";
    return 0;
}
