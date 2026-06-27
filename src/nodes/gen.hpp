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

    std::vector<node*> to_primitives() const override {
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

        printf("out: [%s]\n", json);

        std::vector<node*> v;
        {
            const char* ptr = json;
            const node_info info = json::deserialize<node_info>(ptr);
          
            struct xd {
                node_info info;
                size_t i;
                node* n;
            };
            std::stack<xd> stack;

            stack.push({info, 0, nullptr});

            while (stack.size()) {
                if (stack.top().i == 0) {
                    for (auto entry : info.json_map)
                        printf("%.*s: %c\n", (int)entry.first.size(), (const char*)entry.first.data(), *entry.second);                
                    putchar('\n');
                    stack.top().n = nodes[stack.top().info.type]->deserialize(depth + stack.size() - 1, stack.top().info.json_map);
                }

                if (stack.top().i == stack.top().info.children.size() || stack.top().n->child_nodes() == nullptr) {
                    if (stack.size() == 1) {
                        v.push_back(stack.top().n);
                        stack.pop();
                    }
                    else {
                        node* ptr = stack.top().n;
                        stack.pop();
                        
                        stack.top().n->add_node(ptr);
                    }
                }
                else stack.push({stack.top().info.children[stack.top().i++], 0, nullptr});
            }

            //const std::string type = json::deserialize_field<std::string>(json, "type");
            printf("Node type: %s\nChildren: %zu\n", info.type.c_str(), info.children.size());

            //delete s;
            //v.push_back(nodes[type]->deserialize(json));
            //v.push_back(new sec_node({100}));
            //

            return v;
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
