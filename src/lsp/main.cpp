#include <string_view>
#include <unordered_map>

#include "log.hpp"
#include "message.hpp"
#include "objects.hpp"
#include "tokens.hpp"

std::unordered_map<std::string, std::string> files;


void handle_msg(std::string_view msg) {
    const char* ptr = msg.data();
    const msg::method_t m = json::deserialize<msg::method_t>(ptr);

    log(msg);
    log(m.method);

    if (m.method == "initialize") {
        msg::send("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"capabilities\":{\"textDocumentSync\":1,\"completionProvider\":{},\"semanticTokensProvider\":{\"legend\":{\"tokenTypes\":[\"keyword\"],\"tokenModifiers\":[]},\"full\":true}}}}");
    }
    else if (m.method == "textDocument/didOpen") {
        const text_document doc = json::deserialize_field<params>(msg.data(), "params").text_doc;
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
}

int main() {
    while (true) {
        std::string msg = msg::read();

        if (msg.empty()) continue;

        handle_msg(msg);
    }
}
