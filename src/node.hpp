#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_set>
#include <unordered_map>

//#include "options.hpp"
#include "string_type.hpp"
#include "tree.hpp"
#include "value.hpp"

#include "../../serialize/serialize.hpp"

enum scope_end : uint8_t {
    START, ENDL, ENDSCP
};

struct node {
    std::map<std::string_view, value> meta;
    std::unordered_set<std::string_view> tags;

    virtual const char* type() const = 0;
    virtual uint8_t scope_end() = 0;

    virtual uint32_t type_id() const = 0;

    template <typename T>
    static bool is_type(const node* n) {
        return T::t_id == n->type_id();
    }

    virtual const std::vector<node*>* child_nodes() const = 0;
    virtual void add_node(node*) = 0;
    virtual void parse_tokens(std::vector<modoc::string_type>&&, uint8_t tabs) = 0;

    virtual void parse_verbatim(std::string_view str) {}

    virtual void add_meta(const options_t& meta) {
        for (const auto& entry : meta)
            this->meta[entry.first] = entry.second;
    }
    /*virtual void add_meta(const std::vector<std::pair<std::string_view, value>>& entires) {
        for (const std::)
    }*/

    virtual void final_pass() {}

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

struct node_factory {
    virtual node* instance(uint8_t nesting, const options_t&) = 0;
    virtual node* deserialize(uint8_t depth, const std::unordered_map<std::string_view, const char*>&) { return nullptr; }//= 0;
    virtual void set_node_type_id(uint32_t) const = 0;
    virtual ~node_factory() = default;
};

struct special_node : node {
    //virtual std::vector<node*> expand(const std::vector<node*>& subtree) const = 0;
    virtual std::vector<modoc::uninitialized_tree::unode> expand(modoc::tree& subtree) const = 0;

    virtual bool in_second_pass() const {
        return false;
    }

    bool is_primitive() const override final {
        return false;
    }
};

struct text_node : node {
    inline static uint32_t t_id;

    std::vector<modoc::string_type> tokens{};

    text_node(std::vector<modoc::string_type>&& tokens) {
        this->tokens = std::move(tokens);
    }

    text_node(std::vector<modoc::string_type>&& tokens, uint16_t offset) {
        this->tokens.resize(tokens.size() - offset);
        for (uint16_t i = offset, j = 0; i < tokens.size(); ++i, ++j)
            this->tokens[j] = std::move(tokens[i]);
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

    void parse_tokens(std::vector<modoc::string_type>&& new_tokens, uint8_t) override {
        tokens.reserve(tokens.size() + new_tokens.size());
        for (modoc::string_type& s : new_tokens) tokens.push_back(std::move(s));
    }


    void debug_print() const override {
        printf("[text](%zu)\n", tokens.size());
    }


    const std::vector<node*>* child_nodes() const override {
        return nullptr;
    }

    void add_node(node*) override {}
};

static bool debug_str_match(const char* a, const char* b) {
    while (*a != '\0' && *b != '\0')
        if (*a++ != *b++) return false;
    return true;
}

struct group_node : node {
    inline static uint32_t t_id;
    
    std::vector<node*> nodes;

    virtual uint32_t type_id() const override {
        return t_id;
    }

    const char* type() const override {
        return "group";
    }
    
    uint8_t scope_end() override {
        return scope_end::ENDSCP;
    }


    const std::vector<node*>* child_nodes() const override {
        return &nodes;
    }

    void parse_tokens(std::vector<modoc::string_type>&&, uint8_t) override {}

    void add_node(node* n) override {
        nodes.push_back(n);
    }

    ~group_node() override = default;
};

struct group_f : node_factory {
    std::vector<uint8_t> id;

    node* instance(uint8_t, const options_t&) override {
        return new group_node();
    }
    
    node* deserialize(uint8_t depth, const std::unordered_map<std::string_view, const char*>& map) override {
        group_node* g = new group_node();
        return g;
    }

    void set_node_type_id(uint32_t id) const override {
        group_node::t_id = id;
    }
};

struct sec_node : node {
    inline static uint32_t t_id;
    //inline static std::vector<uint8_t> sec_id;

    std::vector<node*> nodes;
    std::string title;

    uint8_t* id = nullptr;
    uint8_t depth = 0;

    sec_node(std::vector<uint8_t> id_v) {
        depth = /*id_size =*/ id_v.size() - 1;
        id = new uint8_t[depth + 1];//id_size];

        for (uint8_t i = 0; i < id_v.size(); ++i)
            id[i] = id_v[i];
    }

    /*sec_node() = default; // TODO: temp
    void set_id(std::vector<uint8_t> id_v) { // TODO: temp
        id_size = id_v.size();
        id = new uint8_t[id_size];

        for (uint8_t i = 0; i < id_size; ++i)
            id[i] = id_v[i];
    }

    sec_node(uint8_t depth) : depth(depth) {}*/

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

    void parse_tokens(std::vector<modoc::string_type>&& new_tokens, uint8_t tabs) override {
        if (title.empty()) {
            title = new_tokens[0].view();
            for (size_t i = 1; i < new_tokens.size(); ++i) {
                title += ' ';
                title += new_tokens[i].view();
            }
        }
        else if (nodes.size() && debug_str_match(nodes.back()->type(), "text"))
            nodes.back()->parse_tokens(std::move(new_tokens), tabs);
        else
            nodes.push_back(new text_node(std::move(new_tokens)));
    }

    void add_node(node* n) override {
        nodes.push_back(n);
    }


    void debug_print() const override {
        if (id != nullptr) {
            printf("%hhu", id[0]);
            for (uint8_t i = 1; i < depth + 1; ++i)
                printf(".%hhu", id[i]);
            putchar(' ');
        }
        else {
            putchar('?');
            for (uint8_t i = 1; i < depth + 1; ++i)
                fputs(".?", stdout);
            putchar(' ');
        }
        puts(title.c_str());
    }

    void final_pass() override {
        /*const uint8_t nums = depth + 1;

        if (sec_id.size() < nums) sec_id.push_back(0);
        else {
            while (sec_id.size() > nums) sec_id.pop_back();
        }

        ++sec_id.back();

        id = new uint8_t[sec_id.size()];
        memcpy(id, sec_id.data(), sec_id.size());*/
    }

    ~sec_node() override {
        if (id != nullptr) delete[] id;
    }
};

//std::vector<uint8_t> sec_node::sec_id;

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
        //return new sec_node(nesting);
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
    inline static uint32_t t_id;

    std::vector<node*> nodes;

    virtual uint32_t type_id() const override {
        return t_id;
    }


    const char* type() const override {
        return "list";
    }
    
    uint8_t scope_end() override {
        return scope_end::START;
    }


    const std::vector<node*>* child_nodes() const override {
        return &nodes;
    }

    void parse_tokens(std::vector<modoc::string_type>&& new_tokens, uint8_t tabs) override {
        std::string_view first = new_tokens[0].view();
        if (nodes.empty() && first[0] != '-') nodes.push_back(new text_node(std::move(new_tokens)));
        else if (first[0] == '-') {
            if (first.size() > 1) {
                text_node* t = new text_node(std::move(new_tokens));
                first = t->tokens[0].view();
                t->tokens[0] = {{first.data() + 1, first.length() - 1}, false};
                nodes.push_back(t);
            }
            else nodes.push_back(new text_node(std::move(new_tokens), 1));
        }
        else nodes.back()->parse_tokens(std::move(new_tokens), tabs);
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
    inline static uint32_t t_id;

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

    void parse_tokens(std::vector<modoc::string_type>&& new_tokens, uint8_t tabs) override {
        static const std::unordered_set<std::string> types {"char", "short", "int", "long"};
        static const std::unordered_set<std::string> keywords {"if", "else", "return", "throw"};


        tokens.reserve(tokens.size() + new_tokens.size() + 1);

        {
            token_t t {"0", NEWL};
            t.str[0] = tabs;
            tokens.push_back(t);
        }

        for (const modoc::string_type& s : new_tokens) {
            token_t t;
            t.str = s.view();

            if (types.contains(t.str)) t.type = TYPE;
            else if (keywords.contains(t.str)) t.type = KEYWORD;

            tokens.push_back(t);
        }
    }

    virtual void debug_print() const override {
        printf("[code](lang = %s)\n", lang.c_str());
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
        return new code_node((std::string)op.at("lang").string());
    }

    void set_node_type_id(uint32_t id) const override {
        code_node::t_id = id;
    }
};

inline std::unordered_map<std::string_view, node_factory*> node_factories;
