#pragma once

#include "../node.hpp"
#include "../value.hpp"
#include <algorithm>

struct meta_node : special_node {
    inline static uint32_t t_id;

    std::string content;

    const char* type() const override {
        return "meta";
    }

    uint8_t scope_end() override {
        return scope_end::ENDSCP;
    }

    uint32_t type_id() const override {
        return t_id;
    }
 

    /*const std::vector<node*>* child_nodes() const override {
        return nullptr;
    }

    void add_node(node*) override {}*/

    void parse_tokens(std::vector<modoc::string_type>&& tokens, uint8_t depth) override {
        while (depth--) content += '\t';
        for (const modoc::string_type& s : tokens) {
            content += s.view();
            content += ' ';
        }
        content += '\n';
    }   

    bool verbatim() const override {
        return true;
    }


    std::vector<modoc::uninitialized_tree::unode> expand(modoc::tree& subtree) const override {
    //std::vector<node*> expand(const std::vector<node*>& subtree) const override {
        bool def = false;
        const char* begin = 0;

        /*for (const char* ptr = content.c_str(); *ptr != '\0'; ++ptr) {
            if (*ptr > ' ' && !def) {
                def = true;
                begin = ptr;
            }
            else if (*ptr <= ' ' && def) {
                const std::string_view meta_class = {begin, ptr};
                def = false;

                // Meta definitions scope
                while (*ptr <= ' ') ++ptr;

                while (*ptr != '\n' && *ptr != '\0') ++ptr;
                if (*ptr == '\0') break;
                ++ptr;

                options_t map;
                parse_options()
            }
        }*/


        /*auto itr = std::find(subtree.begin(), subtree.end(), this);
        while (itr != subtree.end()) {
            node* n = *itr;
            if (!n->meta.contains("color")) n->meta["color"] = value("yellow"); 
            ++itr;
        }*/
        return {};
    }

    ~meta_node() override = default;
};

struct meta_f : node_factory {
    node* instance(uint8_t depth, const options_t& op) override {
        return new meta_node();
    }

    void set_node_type_id(uint32_t id) const override {
        meta_node::t_id = id;
    }

    ~meta_f() override = default;
};
