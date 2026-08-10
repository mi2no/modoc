#pragma once

#include "../node.hpp"
#include "../tree.hpp"
#include "../value.hpp"

struct repeat_node : special_node {
    inline static uint32_t t_id;

    uint8_t depth;
    double from, to;
    std::string modoc;

    repeat_node(double from, double to, uint8_t depth) : from(from), to(to), depth(depth) {}

    const char* type() const override {
        return "repeat";
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
        while (depth--) modoc += '\t';
        for (const modoc::string_type& s : tokens) {
            modoc += s.view();
            modoc += ' ';
        }
        modoc += '\n';
    }

    /*void debug_print() const override {
        //printf("[%s](cmd = %s)\n", type(), command.c_str());
        printf("[repeat] %s\n", modoc.c_str());
    }*/

    bool verbatim() const override {
        return true;
    }


    std::vector<modoc::uninitialized_tree::unode> expand(modoc::tree& subtree) const override {
    //std::vector<node*> expand(const std::vector<node*>&) const override {
        std::vector<modoc::uninitialized_tree::unode> result;
        result.reserve((size_t)(to - from) * 2);

        for (double i = from; i < to; ++i) {
            result.push_back({{"assign", false}, {"", false}, {"overwrite = true", false}, {"i = 1", false}});
            result.push_back({true});
        }

        //modoc::apply_meta(result, meta);

        return result;
    }

    ~repeat_node() override = default;
};

struct repeat_f : node_factory {
    node* instance(uint8_t depth, const options_t& op) override {
        double from = 0, to = 5; // TODO: replce with 'value' type
        if (op.contains("from") && op.at("from").type == value::NUMBER)
            from = op.at("from").number();
        if (op.contains("to") && op.at("to").type == value::NUMBER)
            to = op.at("to").number();
        return new repeat_node(from, to, depth);
    }

    void set_node_type_id(uint32_t id) const override {
        repeat_node::t_id = id;
    }
};
