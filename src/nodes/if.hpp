#pragma once

#include "../node.hpp"
#include "../tree.hpp"
#include "../value.hpp"

struct if_node : special_node {
    inline static uint32_t t_id;

    bool value;

    if_node(bool value) : value(value) {}

    const char* type() const override {
        return "if";
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
        /*while (depth--) modoc += '\t';
        for (const modoc::string_type& s : tokens) {
            modoc += s.view();
            modoc += ' ';
        }
        modoc += '\n';*/
    }

    bool verbatim() const override {
        return false;
    }


    std::vector<modoc::uninitialized_tree::unode> expand(modoc::tree& subtree) const override {
        if (value) return {{true}};
        else return {};
    }

    ~if_node() override = default;
};

struct if_f : node_factory {
    node* instance(uint8_t depth, const options_t& op) override {
        bool b_value = false;
        if (op.contains("clause")) {
            const value v = op.at("clause");
            if (v.type == value::BOOLEAN) b_value = v.boolean();
        }
        return new if_node(b_value);
    }

    void set_node_type_id(uint32_t id) const override {
        if_node::t_id = id;
    }
};
