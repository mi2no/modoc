#pragma once

#include <cstdio>
#include <cstdlib>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdint>

#include "../node.hpp"
#include "../tree.hpp"

#include "../../../serialize/serialize.hpp"

struct gen_node : special_node {
    static uint32_t t_id;
    
    uint8_t depth;
    std::string command;

    enum mode_e : uint8_t {
        JSON, MODOC
    };

    uint8_t mode;

    gen_node(uint8_t depth, std::string_view command, uint8_t mode) : depth(depth), command(command), mode(mode) {}


    const char* type() const override {
        return "gen";
    }

    uint8_t scope_end() override {
        return START;
    }


    uint32_t type_id() const override {
        return t_id;
    }


    const std::vector<node*>* child_nodes() const override {
        return nullptr;
    }

    void add_node(node*) override {}

    void parse_tokens(const std::vector<std::string_view>&, uint8_t) override {}

    void debug_print() const override {
        printf("[%s](cmd = %s)\n", type(), command.c_str());
    }

//private:

    struct node_info {
        std::string type;
        std::vector<std::string> tags;
        std::vector<node_info> children;
        
        std::string text; // used if node is a text node
        bool is_text = false;

        std::unordered_map<std::string_view, const char*> json_map;
    };

public:

    std::vector<node*> expand() const override {
        char path[] = "tmp_modoc_outXXXXXX";
        int fd = mkstemp(path);

        const std::string env_cmd = std::string("MODOC_OUTPUT_FILE='") + path + "' " + command;
        system(env_cmd.c_str());

        struct stat st;
        fstat(fd, &st);

        char* out = new char[st.st_size + 1];
        read(fd, out, st.st_size);
        out[st.st_size] = '\0';

        close(fd);
        unlink(path);

        std::vector<node*> v;
        switch (mode) {
            case MODOC:
                puts("[@gen] modoc:");
                puts(out);

                return modoc::create_tree(out, depth);
            case JSON:
            {
                puts("[@gen] json:");
                json::pretty_print(out);

                const char* ptr = out;
                std::vector<node_info> subtree;
                json::deserialize_value(subtree, ptr);

                struct node_tree_itr { // TODO: change name
                    const std::vector<node_info>& infos;
                    size_t ind;
                    node* parent;
                };
                std::stack<node_tree_itr> stack;

                stack.push({subtree, 0, nullptr});

                while (stack.size()) {
                    node_tree_itr* top = &stack.top();
                    const node_info& info = top->infos[top->ind];
                    node* curr;

                    if (info.is_text) {
                        std::vector<std::string_view> tokens = modoc::tokenize(info.text);
                        //for (std::string_view sv : tokens) printf("%.*s\n", (int)sv.size(), sv.data());
                        curr = new text_node(tokens);
                    }
                    else if (nodes.contains(info.type)) {
                        curr = nodes[info.type]->deserialize(depth + stack.size() - 1, top->infos[top->ind].json_map);
                    }
                    else if (info.type.empty()) {
                        curr = nodes["group"]->deserialize(depth + stack.size() - 1, top->infos[top->ind].json_map);
                    }
                    else {
                        while (++top->ind == top->infos.size()) {
                            node* const parent = top->parent;
                            stack.pop();

                            if (stack.empty()) break;

                            top = &stack.top();
                            if (top->parent == nullptr) v.push_back(parent);
                            else top->parent->add_node(parent);
                        }
                        continue;
                    }

                    if (top->infos[top->ind].children.empty() || curr->child_nodes() == nullptr) { // Doesn't have or expect child nodes
                        if (top->parent == nullptr) v.push_back(curr);
                        else top->parent->add_node(curr);

                        while (++top->ind == top->infos.size()) {
                            node* const parent = top->parent;
                            stack.pop();

                            if (stack.empty()) break;

                            top = &stack.top();
                            if (top->parent == nullptr) v.push_back(parent);
                            else top->parent->add_node(parent);
                        }
                    }
                    else stack.push({top->infos[top->ind].children, 0, curr});
                }
            }
        }

        delete[] out;

        puts("Evaluated subtree:");
        modoc::print_doc_tree(v);

        return v;
    }

    ~gen_node() override = default;
};

struct gen_f : node_factory {
    node* instance(uint8_t depth, const options_t& op) override {
        if (op.contains("mode") && op.at("mode").view == "modoc") return new gen_node(depth, op.at("cmd").view, gen_node::MODOC);
        else return new gen_node(depth, op.at("cmd").view, gen_node::JSON);
    }

    void set_node_type_id(uint32_t id) const override {
        gen_node::t_id = id;
    }
};

template <>
struct serializer<gen_node::node_info> {
    using self = gen_node::node_info;
    using fields = field_list<
        AUTO_FIELD(type),
        AUTO_FIELD(tags),
        AUTO_FIELD(children)
    >;

    static void post_deserialize(self& info, std::unordered_map<std::string_view, const char*>& fields) {
        info.json_map = std::move(fields);
    }

    static self from_string(std::string_view view) {
        self info;
        info.text = view;
        info.is_text = true;
        return info;
    }
};

uint32_t gen_node::t_id = 0;
