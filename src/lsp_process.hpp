#pragma once

#include "util.hpp"
#include <charconv>
#include <cstring>
#include <string>
#include <string_view>
#include <unistd.h>
#include <sys/wait.h>

namespace modoc {
    struct lsp_process {
        std::string name;
        pid_t pid;

        int to_lsp[2];
        int from_lsp[2];

        lsp_process(std::string_view name) : name(name) {}

        void write_all(int fd, std::string_view str) {
            const char* data = str.data();
            size_t size = str.size();

            while (size > 0) {
                const ssize_t result = write(fd, data, size);

                if (result <= 0) {
                    // handle error
                }

                data += result;
                size -= result;
            }
        }

        void send(std::string_view json) {
            std::string header = std::string("Content-Length: ") + std::to_string(json.size()) + "\r\n\r\n";
        
            write_all(to_lsp[1], header);
            write_all(to_lsp[1], json);
        }

        static std::string_view extract_message(std::string_view buffer) {
            constexpr size_t header_size = modoc::string_len("Content-Length: ");
            const size_t header_end = buffer.find("\r\n\r\n");

            if (header_end == std::string_view::npos) return {};

            size_t size;
            std::from_chars(buffer.data() + header_size, buffer.data() + header_end, size);

            const char* content_begin = buffer.data() + header_end + 4;

            if (content_begin + size > buffer.end()) return {};

            return {content_begin, content_begin + size};
        }
       
        std::string buffer;

        std::string receive() {
            constexpr size_t size = 4096;
            char temp_buff[size];

            while (true) {
                std::string_view msg;
                if (buffer.size()) msg = extract_message(buffer);
                
                if (msg.size()) {
                    std::string res = std::string(msg);
                    buffer.erase(0, msg.end() - buffer.data());
                    return res;
                }

                const ssize_t count = read(from_lsp[0], temp_buff, size);
                buffer += {temp_buff, temp_buff + count};
            }
        }


        void init() {
            pipe(to_lsp);
            pipe(from_lsp);

            pid = fork();

            if (pid == 0) { // Child
                dup2(to_lsp[0], STDIN_FILENO);
                dup2(from_lsp[1], STDOUT_FILENO);

                close(to_lsp[0]);
                close(to_lsp[1]);

                close(from_lsp[0]);
                close(from_lsp[1]);

                execlp(name.c_str(), name.c_str(), nullptr); // Replace with lsp process 
            }
            else {
                close(to_lsp[0]);
                close(from_lsp[1]);

                static std::string_view json_init = "{\"jsonrpc\": \"2.0\",\"id\": 1,\"method\": \"initialize\",\"params\": {}}";
                send(json_init);
                receive();
            }
        }

        void shutdown() {
            static std::string_view json_shutdown = "{\"jsonrpc\": \"2.0\",\"id\": 123,\"method\": \"shutdown\",\"params\": null}";
            static std::string_view json_exit = "{\"jsonrpc\": \"2.0\",\"method\": \"exit\"}";

            send(json_shutdown);
            receive();
            send(json_exit);

            close(to_lsp[1]);
            close(from_lsp[0]);
            waitpid(pid, nullptr, 0);
        }
    };
}
