#pragma once

#include "../node.hpp"
#include "../tree.hpp"
#include "../value.hpp"
#include "../log.hpp"

struct title_node : special_node {
    inline static uint32_t t_id;

    std::string title, author, date;

    title_node(std::string_view title, std::string_view author, std::string_view date) : title(title), author(author), date(date) {}

    const char* type() const override {
        return "title";
    }

    uint8_t scope_end() override {
        return scope_end::START;
    }


    uint32_t type_id() const override {
        return t_id;
    }


    /*const std::vector<node*>* child_nodes() const override {
        return nullptr;
    }

    void add_node(node*) override {}*/


    /*void debug_print() const override {
        //printf("[%s](cmd = %s)\n", type(), command.c_str());
        printf("[repeat] %s\n", modoc.c_str());
    }*/

    void parse_tokens(std::vector<modoc::string_type>&& tokens, uint8_t depth) override {}

    std::vector<modoc::uninitialized_tree::unode> expand(modoc::tree& subtree) const override {
        std::vector<modoc::uninitialized_tree::unode> result;

        modoc::uninitialized_tree::unode group = {{"group", false}, {"", false}, {"", false}, {"", false}};

        modoc::uninitialized_tree::unode title_n = {{title, false}};
        modoc::uninitialized_tree::unode author_n = {{author, false}};
        modoc::uninitialized_tree::unode date_n = {{date, false}};

        group.node().children.push_back(std::move(title_n));
        group.node().children.push_back(std::move(author_n));
        group.node().children.push_back(std::move(date_n));

        std::cout << "Title group: " << group.node().children.size() << '\n';

        result.push_back(std::move(group));
        return result;
    }

    ~title_node() override = default;
};

struct title_f : node_factory {
    node* instance(modoc::tree& parent, uint8_t depth, const options_t& op) override {
        std::string_view title, author, date;
        const value* ptr = parent.get_variable("title");

        if (ptr != nullptr) title = ptr->string();

        ptr = parent.get_variable("author", value::STRING);
        if (ptr != nullptr) author = ptr->string();

        ptr = parent.get_variable("date", value::STRING);
        if (ptr != nullptr) date = ptr->string();

        {
            std::string msg = "title: ";
            msg += title;
            msg += "\nauthor: ";
            msg += author;
            msg += "\ndate: ";
            msg += date;

            modoc::logger::s_log("title", "init", msg);
        }

        return new title_node(title, author, date);
    }

    void set_node_type_id(uint32_t id) const override {
        title_node::t_id = id;
    }
};
