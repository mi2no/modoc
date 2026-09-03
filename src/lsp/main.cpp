#include <string_view>
#include <unordered_map>

#include "log.hpp"
#include "message.hpp"
#include "objects.hpp"
#include "tokens.hpp"

std::unordered_map<std::string, std::string> files;

std::string temp_format_escape_chars(std::string&& str) {
    uint32_t escape_chars = 0;
    
    {
        bool slash = false;
        for (const char c : str) {
            if (c == '\\' && !slash) slash = true;
            else if (slash && (c == 'n' || c == '"' || c == 't' || c == '\\')) {
                slash = false;
                ++escape_chars;
            }
        }
    }
    
    if (escape_chars) {
        std::string result;
        result.resize(str.size() - escape_chars);

        bool slash = false;
        char* ptr = result.data();
        for (const char c : str) {
            if (c == '\\' && !slash) slash = true;
            else if (slash) {
                switch (c) {
                    case 'n':
                        *ptr = '\n';
                        break;
                    case '"':
                        *ptr = '"';
                        break;
                    case 't':
                        *ptr = '\t';
                        break;
                    case '\\':
                        *ptr = '\\';
                        break;
                }                    
                slash = false;
                ++ptr;
            }
            else *ptr++ = c;
        }

        return result;
    }
    return str;    
}

constexpr const char* init = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"capabilities\":{\"textDocumentSync\":1,\"completionProvider\":{},\"semanticTokensProvider\":{\"legend\":{\"tokenTypes\":[\"keyword\",\"operator\",\"string\",\"number\",\"comment\",\"variable\",\"property\",\"parameter\",\"boolean\",\"node\"],\"tokenModifiers\":[]},\"full\":true}}}}";

bool handle_msg(std::string_view msg) {
    const char* ptr = msg.data();
    const msg::method_t m = json::deserialize<msg::method_t>(ptr);

    log(msg);
    log(m.method);

    if (m.method == "initialize") {
        msg::send(init);
    }
    else if (m.method == "textDocument/didOpen") {
        text_document doc = json::deserialize_field<params>(msg.data(), "params").text_doc;
        doc.text = temp_format_escape_chars(std::move(doc.text));
        files[doc.uri] = doc.text;
        log(doc.text);
    }
    else if (m.method == "textDocument/completion") {
        position pos = json::deserialize_field<params>(msg.data(), "params").pos;
        log.f("line: %u, char: %u", pos.line, pos.character);
    }
    else if (m.method == "textDocument/semanticTokens/full") {
        const text_document doc = json::deserialize_field<params>(msg.data(), "params").text_doc;
        std::string json = tokenize_json(files[doc.uri].c_str(), m.id);
        log(json);
        msg::send(json);
    }
    else if (m.method == "shutdown") {
        msg::send(std::string("{\"jsonrpc\":\"2.0\",\"id\":") + std::to_string(m.id) + ",\"result\":null}");
    }
    else if (m.method == "exit") return false;

    return true;
}

int main() {
    std::string msg;
    do {
        msg = msg::read();

        if (msg.empty()) continue;

    } while (handle_msg(msg));

}
