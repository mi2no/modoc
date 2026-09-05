#pragma once

#include "util.hpp"
#include <charconv>
#include <cstring>
#include <string>
#include <string_view>
#include <unistd.h>
#include <sys/wait.h>
#include <map>

#include "serialize/serialize.hpp"

namespace modoc {
    struct lsp_process {

        static constexpr std::string_view json_initialize = 
        "{\
            \"jsonrpc\": \"2.0\",\
            \"id\": 1,\
            \"method\": \"initialize\",\
            \"params\": {\
                \"processId\": null,\
                \"rootUri\": null,\
                \"capabilities\": {\
                    \"textDocument\": {\
                        \"semanticTokens\": {\
                            \"requests\": {\
                                \"full\": true\
                            }\
                        }\
                    }\
                }\
            }\
        }";

        static constexpr std::string_view json_initialized = 
        "{\
            \"jsonrpc\": \"2.0\",\
            \"method\": \"initialized\",\
            \"params\": {}\
        }";

        std::string name;
        pid_t pid;

        int to_lsp[2];
        int from_lsp[2];

        std::vector<std::string> token_types, token_modifiers;
        std::map<std::string, std::string_view, std::less<>> docs;

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


        void init(std::string_view lsp_name) {
            name = lsp_name;

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

                send(json_initialize);
                std::string msg = receive();

                puts(json::pretty(msg.c_str()).c_str());
                {
                    ssize_t idx = msg.find("legend");

                    if (idx != std::string::npos) {
                        const char* ptr = msg.data() + idx;
                        while (*ptr != '{') ++ptr;

                        token_types = json::deserialize_field<std::vector<std::string>>(ptr, "tokenTypes");
                        token_modifiers = json::deserialize_field<std::vector<std::string>>(ptr, "tokenModifiers");
                    }
                }
                
                send(json_initialized);
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

        
        struct txt_doc_info {
            std::string_view uri, language_id, text;
            uint32_t version;
        };

        void open_document(txt_doc_info info) {
            static constexpr std::string_view json_did_open = 
"{\
\"jsonrpc\": \"2.0\",\
\"method\": \"textDocument/didOpen\",\
\"params\": {\
\"textDocument\":";

            std::string result = std::string(json_did_open) +  json::serialize_struct(info) + "}}";
            puts(json::pretty(result.c_str()).c_str());

            docs[std::string(info.uri)] = info.text;

            send(result);
        }

        void close_document(std::string_view uri) {
            static constexpr std::string_view json_did_close = 
"{\
\"jsonrpc\": \"2.0\",\
\"method\": \"textDocument/didClose\",\
\"params\": {\
\"textDocument\": {\
\"uri\": \"";

            std::string msg = std::string(json_did_close);
            msg += uri;
            msg += "\"}}}";
            send(msg);
        }
        
        struct token {
            std::string_view str;
            uint16_t type_id;
            uint32_t modifiers;
        };

        std::vector<token> request_tokens(std::string_view uri) {
            static constexpr std::string_view json_semantic =  
                "{\
                    \"jsonrpc\": \"2.0\",\
                    \"id\": 2,\
                    \"method\": \"textDocument/semanticTokens/full\",\
                    \"params\": {\
                        \"textDocument\": {\
                            \"uri\":\"";
            
            std::string msg = std::string(json_semantic);
            msg += uri;
            msg += "\"}}}";

            send(msg);
            receive();
            
            std::string token_msg = receive();
            puts(json::pretty(token_msg.c_str()).c_str());

            const ssize_t idx = token_msg.find("data");

            if (idx == std::string::npos) return {};

            const char* ptr = token_msg.data() + idx;
            while (*ptr != '[') ++ptr;

            std::vector<uint32_t> nums = {};
            json::deserialize_value(nums, ptr);

            ptr = docs.find(uri)->second.data();
            token t;
            std::vector<token> tokens;
            for (size_t i = 0; i < nums.size();) {
                {
                    uint32_t delta_lines = nums[i++];
                    while (delta_lines)
                        if (*ptr++ == '\n') --delta_lines;
                }
                {
                    uint32_t delta_cols = nums[i++];
                    ptr += delta_cols;
                }
                t.str = {ptr, ptr + nums[i++]};
                t.type_id = nums[i++];
                t.modifiers = nums[i++];
                tokens.push_back(t);
            }

        for (const auto& t : tokens) {
                std::cout << t.str << ' ' << token_types[t.type_id] << " |";

                uint32_t mods = t.modifiers;
                uint16_t id = 0;
                while (mods) {
                    const uint16_t off = std::countr_zero(mods);
                    std::cout << ' ' << token_modifiers[id += off];
                    
                    mods >>= (off + 1);
                    ++id;
                }
                putchar('\n');
            }


            return tokens;
        }
    };
}

template <>
struct serializer<modoc::lsp_process::txt_doc_info> {
    using self = modoc::lsp_process::txt_doc_info;
    using fields = field_list<
        AUTO_FIELD(uri),
        AUTO_FIELD(text),
        AUTO_FIELD(version),
        field<&self::language_id, "languageId">
    >;
};
