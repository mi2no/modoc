#pragma once

#include <string>

#include "../node.hpp"
#include "../tree.hpp"
#include "../value.hpp"
#include "../log.hpp"

struct assign_node : special_node {
    inline static uint32_t t_id;

    modoc::string_type content;
    //std::string_view content;
    bool overwrite;

    assign_node(bool overwrite) : overwrite(overwrite) {}

    const char* type() const override {
        return "assign";
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
        //for (const modoc::string_type& s : tokens) content += s.view();
    }

    void parse_verbatim(std::string_view str, bool to_copy) override {
        content = {str, to_copy};
    }

    /*void debug_print() const override {
        printf("[assign] %s\n", content.c_str());
    }*/

    bool verbatim() const override {
        return true;
    }

    std::vector<modoc::uninitialized_tree::unode> expand(modoc::tree& subtree) const override {
        options_t assigned; 
        parse_options(content.view(), assigned);

        modoc::logger log;
        std::string msg;

        for (auto& entry : assigned) {
            msg += std::string(entry.first);
            msg += " = ";
            msg += entry.second.to_string();
            msg += '\n';

            if (content.is_owned()) {
                value v = entry.second; 
                subtree.assign(entry.first, std::move(v), overwrite);
            }
            else subtree.assign(entry.first, std::move(entry.second), overwrite);
            
            //entry.second.type = value::NONE;
            //variables[entry.first] = entry.second;
        } // TODO: use function of subtree to add variables
        assigned.clear();

        log.log("assign", "expand", msg);

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
        return new assign_node(overwrite);
    }

    void set_node_type_id(uint32_t id) const override {
        assign_node::t_id = id;
    }
};
