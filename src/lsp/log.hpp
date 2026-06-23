#pragma once
#include <cstdio>
#include <ctime>
#include <string_view>

class log_t {
    FILE* const file = fopen("lsp.log", "a");

public:
    void operator()(std::string_view str) {
        const time_t now = time(nullptr);
        tm* time = localtime(&now);

        fprintf(file, "[%hhu:%hhu:%hhu] %s\n", time->tm_hour, time->tm_min, time->tm_sec, str.data());
        fflush(file);
    }

    template <typename... T>
    void f(std::string_view format, T... args) {
        const time_t now = time(nullptr);
        tm* time = localtime(&now);

        fprintf(file, "[%hhu:%hhu:%hhu] ", time->tm_hour, time->tm_min, time->tm_sec);
        fprintf(file, format.data(), args...);
        fputc('\n', file);
        fflush(file);
    }

    ~log_t() {
        fclose(file);
    }
} static log;
