#pragma once

#include "../node.hpp"
#include "../value.hpp"
#include <algorithm>

struct meta_node : special_node {
    static uint32_t t_id;

    const char* type() const override {
        return "meta";
    }

    uint8_t scope_end() override {
        return scope_end::START;
    }

    uint32_t type_id() const override {
        return t_id;
    }
 

    const std::vector<node*>* child_nodes() const override {
        return nullptr;
    }

    void add_node(node*) override {}

    void parse_tokens(std::vector<modoc::string_type>&&, uint8_t) override {}   


    std::vector<node*> expand(const std::vector<node*>& subtree) const override {
        auto itr = std::find(subtree.begin(), subtree.end(), this);
        while (itr != subtree.end()) {
            node* n = *itr;
            if (!n->meta.contains("color")) n->meta["color"] = value("yellow"); 
            ++itr;
        }
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

uint32_t meta_node::t_id = 0;
