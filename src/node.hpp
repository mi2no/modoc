#pragma once
#include <cstdint>
#include <cstdio>
#include <vector>
#include <string_view>
#include <unordered_set>
#include <unordered_map>

#include "options.hpp"

enum scope_end : uint8_t {
    START, ENDL, ENDSCP
};

struct node {    
    virtual const char* type() const = 0;
    virtual uint8_t scope_end() = 0;

    virtual uint32_t type_id() const = 0;

    template <typename T>
    static bool is_type(const node* n) {
        return T::t_id == n->type_id();
    }

    virtual const std::vector<node*>* child_nodes() const = 0;
    virtual void add_node(node*) = 0;
    virtual void parse_tokens(const std::vector<std::string_view>&, uint8_t tabs) = 0;

    virtual void debug_print() const {
        printf("[%s]\n", type());
    }

    virtual bool verbatim() const {
        return false;
    }

    virtual bool is_primitive() const {
        return true;
    }
    
    virtual ~node() = default;
};

struct special_node : node {
    virtual std::vector<node*> to_primitives() const = 0;

    bool is_primitive() const override final {
        return false;
    }
};

struct text_node : node {
    static uint32_t t_id;

    std::vector<std::string_view> tokens{};

    text_node(const std::vector<std::string_view>& tokens) {
        this->tokens = tokens;
    }

    text_node(const std::vector<std::string_view>& tokens, uint16_t offset) {
        this->tokens.resize(tokens.size() - offset);
        for (uint16_t i = offset, j = 0; i < tokens.size(); ++i, ++j)
            this->tokens[j] = tokens[i];
    }


    virtual uint32_t type_id() const override {
        return t_id;
    }


    const char* type() const override {
        return "text";
    }

    uint8_t scope_end() override {
        return scope_end::ENDSCP;
    }

    void parse_tokens(const std::vector<std::string_view>& new_tokens, uint8_t) override {
        tokens.reserve(tokens.size() + new_tokens.size());
        for (const std::string_view& s : new_tokens) tokens.push_back(s);
    }


    const std::vector<node*>* child_nodes() const override {
        return nullptr;
    }

    void add_node(node*) override {}
};

struct node_factory {
    virtual node* instance(uint8_t nesting, const options_t&) = 0;
    virtual node* deserialize(uint8_t depth, const std::unordered_map<std::string_view, const char*>&) { return nullptr; }//= 0;
    virtual void set_node_type_id(uint32_t) const = 0;
    virtual ~node_factory() = default;
};

static bool debug_str_match(const char* a, const char* b) {
    while (*a != '\0' && *b != '\0')
        if (*a++ != *b++) return false;
    return true;
}

struct sec_node : node {
    static uint32_t t_id;

    std::vector<node*> nodes;
    std::string title;

    uint8_t* id = nullptr;
    uint8_t id_size = 0;

    sec_node(std::vector<uint8_t> id_v) {
        id_size = id_v.size();
        id = new uint8_t[id_size];

        for (uint8_t i = 0; i < id_size; ++i)
            id[i] = id_v[i];
    }

    sec_node() = default; // TODO: temp
    void set_id(std::vector<uint8_t> id_v) { // TODO: temp
        id_size = id_v.size();
        id = new uint8_t[id_size];

        for (uint8_t i = 0; i < id_size; ++i)
            id[i] = id_v[i];
    }

    virtual uint32_t type_id() const override {
        return t_id;
    }

    const char* type() const override {
        return "section";
    }
    
    uint8_t scope_end() override {
        return scope_end::ENDL;
    }


    const std::vector<node*>* child_nodes() const override {
        return &nodes;
    }

    void parse_tokens(const std::vector<std::string_view>& new_tokens, uint8_t tabs) override {
        if (title.empty()) {
            title = new_tokens[0];
            for (size_t i = 1; i < new_tokens.size(); ++i) {
                title += ' ';
                title += new_tokens[i];
            }
        }
        else if (nodes.size() && debug_str_match(nodes.back()->type(), "text"))
            nodes.back()->parse_tokens(new_tokens, tabs);
        else
            nodes.push_back(new text_node(new_tokens));
    }

    void add_node(node* n) override {
        nodes.push_back(n);
    }


    void debug_print() const override {
        if (id != nullptr) printf("%hhu", id[0]);
        for (uint8_t i = 1; i < id_size; ++i)
            printf(".%hhu", id[i]);
        putchar(' ');
        puts(title.c_str());
    }

    ~sec_node() override {
        if (id != nullptr) delete[] id;
    }
};

struct sec_f : node_factory {
    std::vector<uint8_t> id;

    void handle_depth(uint8_t depth) {
        const uint8_t sec_depth = depth + 1;

        if (sec_depth > id.size()) id.push_back(0u);
        else {
            while (sec_depth < id.size()) id.pop_back();
        }

        ++id.back();
    }

    node* instance(uint8_t nesting, const options_t&) override {
        handle_depth(nesting);
        return new sec_node(id);
    }
    
    node* deserialize(uint8_t depth, const std::unordered_map<std::string_view, const char*>& map) override {
        handle_depth(depth);

        sec_node* s = new sec_node(id);

        if (map.contains("title")) {
            const char* ptr = map.at("title");
            json::deserialize_value(s->title, ptr);
        }

        return s;
    }

    void set_node_type_id(uint32_t id) const override {
        sec_node::t_id = id;
    }
};
    
template <>
struct serializer<sec_node> {
    using self = sec_node;
    using fields = field_list<
        AUTO_FIELD(title)
    >;
};

struct list_node : node {
    static uint32_t t_id;

    std::vector<node*> nodes;

    virtual uint32_t type_id() const override {
        return t_id;
    }


    const char* type() const override {
        return "list";
    }
    
    uint8_t scope_end() override {
        return scope_end::ENDL;
    }


    const std::vector<node*>* child_nodes() const override {
        return &nodes;
    }

    void parse_tokens(const std::vector<std::string_view>& new_tokens, uint8_t tabs) override {
        if (nodes.empty() && new_tokens[0][0] != '-') nodes.push_back(new text_node(new_tokens));
        else if (new_tokens[0][0] == '-') {
            if (new_tokens[0].size() > 1) {
                text_node* t = new text_node(new_tokens);
                t->tokens[0] = {t->tokens[0].data() + 1, t->tokens[0].length() - 1};
                nodes.push_back(t);
            }
            else nodes.push_back(new text_node(new_tokens, 1));
        }
        else nodes.back()->parse_tokens(new_tokens, tabs);
    }

    void add_node(node* n) override {
        nodes.push_back(n);
    }
};

struct list_f : node_factory {
    node* instance(uint8_t, const options_t&) override {
        return new list_node();
    }

    void set_node_type_id(uint32_t id) const override {
        list_node::t_id = id;
    }
};

struct code_node : node {
    static uint32_t t_id;

    enum token_type : uint8_t {
        NEWL, NONE, KEYWORD, TYPE
    };

    struct token_t {
        std::string str;
        token_type type = NONE;
    };

    std::vector<token_t> tokens{};
    std::string lang;

    code_node(std::string lang) : lang(lang) {}

    virtual uint32_t type_id() const override {
        return code_node::t_id;
    }


    const char* type() const override {
        return "code";
    }

    uint8_t scope_end() override {
        return scope_end::ENDSCP;
    }

    void parse_tokens(const std::vector<std::string_view>& new_tokens, uint8_t tabs) override {
        static const std::unordered_set<std::string> types {"char", "short", "int", "long"};
        static const std::unordered_set<std::string> keywords {"if", "else", "return", "throw"};


        tokens.reserve(tokens.size() + new_tokens.size() + 1);

        {
            token_t t {"0", NEWL};
            t.str[0] = tabs;
            tokens.push_back(t);
        }

        for (const std::string_view& s : new_tokens) {
            token_t t;
            t.str = s;

            if (types.contains(t.str)) t.type = TYPE;
            else if (keywords.contains(t.str)) t.type = KEYWORD;

            tokens.push_back(t);
        }
    }


    const std::vector<node*>* child_nodes() const override {
        return nullptr;
    }

    bool verbatim() const override {
        return true;
    }

    void add_node(node*) override {}

};

struct code_f : node_factory {
    node* instance(uint8_t, const options_t& op) override {
        return new code_node((std::string)op.at("lang").view);
    }

    void set_node_type_id(uint32_t id) const override {
        code_node::t_id = id;
    }
};

uint32_t text_node::t_id = 0;
uint32_t sec_node::t_id = 0;
uint32_t list_node::t_id = 0;
uint32_t code_node::t_id = 0;

static std::unordered_map<std::string_view, node_factory*> nodes;
