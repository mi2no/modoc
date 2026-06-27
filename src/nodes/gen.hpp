#pragma once

#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdint>

#include "../node.hpp"
#include "../../../serialize/serialize.hpp"

struct gen_node : special_node {
    static uint32_t t_id;
    
    uint8_t depth;
    std::string command;

    gen_node(uint8_t depth, std::string_view command) : depth(depth), command(command) {}


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
        std::vector<node_info> children;

        std::unordered_map<std::string_view, const char*> json_map;
    };

public:

    std::vector<node*> expand() const override {
        char path[] = "tmp_json_XXXXXX";
        int fd = mkstemp(path);

        const std::string env_cmd = std::string("MODOC_JSON_FILE='") + path + "' " + command;
        system(env_cmd.c_str());

        struct stat st;
        fstat(fd, &st);

        char* json = new char[st.st_size + 1];
        read(fd, json, st.st_size);
        json[st.st_size] = '\0';

        close(fd);
        unlink(path);

        puts("[@gen] json:");
        json::pretty_print(json);

        std::vector<node*> v;
        {
            const char* ptr = json;
            std::vector<node_info> subtree;
            json::deserialize_value(subtree, ptr);

            struct xd {
                const std::vector<node_info>& infos;
                size_t ind;
                node* parent;
            };
            std::stack<xd> stack;

            stack.push({subtree, 0, nullptr});

            while (stack.size()) {
                xd* top = &stack.top(); 
                node* curr = nodes[top->infos[top->ind].type]->deserialize(depth + stack.size() - 1, top->infos[top->ind].json_map);

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

        delete[] json;
        return v;
    }

    ~gen_node() override = default;
};

struct gen_f : node_factory {
    node* instance(uint8_t depth, const options_t& op) override {
        return new gen_node(depth, op.at("cmd").view);
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
        AUTO_FIELD(children)
    >;

    static void post_deserialize(self& info, std::unordered_map<std::string_view, const char*>& fields) {
        info.json_map = std::move(fields);
    }
};

uint32_t gen_node::t_id = 0;
