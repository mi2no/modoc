#pragma once

#include "../node.hpp"

struct gen_node : node {
    static uint32_t t_id;
    
    std::string command;

    gen_node(std::string_view command) : command(command) {}


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


    ~gen_node() override = default;
};

struct gen_f : node_factory {
    node* instance(uint8_t, const options_t& op) override {
        return new gen_node(op.at("cmd").view);
    }

    void set_node_type_id(uint32_t id) const override {
        gen_node::t_id = id;
    }
};

uint32_t gen_node::t_id = 0;
