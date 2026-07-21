#pragma once

#include <string>

#include "../node.hpp"
#include "../tree.hpp"
#include "../value.hpp"

struct assign_node : special_node {
    inline static uint32_t t_id;

    std::string content;

    const char* type() const override {
        return "assign";
    }

    uint8_t scope_end() override {
        return scope_end::ENDSCP;
    }


    uint32_t type_id() const override {
        return t_id;
    }


    const std::vector<node*>* child_nodes() const override {
        return nullptr;
    }

    void add_node(node*) override {}

    void parse_tokens(std::vector<modoc::string_type>&& tokens, uint8_t depth) override {
        for (const modoc::string_type& s : tokens) content += s.view();
    }

    void parse_verbatim(std::string_view str) override {
        content = str;
    }

    void debug_print() const override {
        printf("[assign] %s\n", content.c_str());
    }

    bool verbatim() const override {
        return true;
    }

    std::vector<modoc::uninitialized_tree::unode> expand(modoc::tree& subtree) const override {
        options_t assigned; 
        parse_options(content, assigned);

        for (auto& entry : assigned) {
            printf("[assign] %.*s = %s\n", (int)entry.first.size(), entry.first.data(), entry.second.to_string().c_str());
            subtree.assign(entry.first, std::move(entry.second));
            //variables[entry.first] = entry.second;
        } // TODO: use function of subtree to add variables

        return {};
    }

    ~assign_node() override = default;
};

struct assign_f : node_factory {
    node* instance(uint8_t depth, const options_t& op) override {
        bool overwrite = false;
        if (op.contains("overwrite")) {
            const value& v = op.at("overwrite");
            if (v.type == value::BOOLEAN) overwrite = v.boolean();
        }
        return new assign_node();
    }

    void set_node_type_id(uint32_t id) const override {
        assign_node::t_id = id;
    }
};
