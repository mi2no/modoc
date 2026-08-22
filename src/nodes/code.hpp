#pragma once

#include <string>

#include "../node.hpp"

struct new_code_node : special_node {
    inline static uint32_t t_id = 0;

    enum token_type : uint8_t {
        NEWL, NONE, KEYWORD, TYPE
    };

    struct token_t {
        std::string str;
        token_type type = NONE;
    };

    std::vector<token_t> tokens{};
    std::string lang;

    new_code_node(std::string lang) : lang(lang) {}

    virtual uint32_t type_id() const override {
        return new_code_node::t_id;
    }


    const char* type() const override {
        return "new_code";
    }

    uint8_t scope_end() override {
        return scope_end::ENDSCP;
    }

    void parse_tokens(std::vector<modoc::string_type>&& new_tokens, uint8_t tabs) override {
        static const std::unordered_set<std::string> types {"char", "short", "int", "long"};
        static const std::unordered_set<std::string> keywords {"if", "else", "return", "throw"};


        tokens.reserve(tokens.size() + new_tokens.size() + 1);

        {
            token_t t {"0", NEWL};
            t.str[0] = tabs;
            tokens.push_back(t);
        }

        for (const modoc::string_type& s : new_tokens) {
            token_t t;
            t.str = s.view();

            if (types.contains(t.str)) t.type = TYPE;
            else if (keywords.contains(t.str)) t.type = KEYWORD;

            tokens.push_back(t);
        }
    }

    /*virtual void debug_print() const override {
        printf("[new_code](lang = %s)\n", lang.c_str());
    }

    const std::vector<node*>* child_nodes() const override {
        return nullptr;
    }*/

    bool verbatim() const override {
        return true;
    }

    //void add_node(node*) override {}


    std::vector<modoc::uninitialized_tree::unode> expand(modoc::tree& subtree) const override {
    //std::vector<node*> expand(const std::vector<node*>&) const override {
        /*std::vector<node*> result {new group_node()};
        group_node *top = (group_node*)result.front(), *last = nullptr;

        top->meta["border-radius"] = {"10px"};
        top->meta["background"] = {"#121212"};
        top->meta["padding"] = {"20px"};
        top->meta["border"] = {"solid 1px var(--sec-color)"};
        top->meta["font-family"] = {"'JetBrains Mono'"};
        top->meta["font-size"] = {".8em"};
        top->meta["overflow"] = {"hidden"};
        top->meta["margin-inline"] = {"calc(var(--heading-size) / 2)"};

        for (const token_t& t : tokens) {
            if (t.type == NEWL) {
                last = new group_node();
                last->meta["display"] = {"flex"};
                last->meta["gap"] = {"1ch"};
                last->meta["padding-left"] = {std::to_string(4 * (uint8_t)t.str[0]) + "ch"};
                top->add_node(last);
            }
            else {
                modoc::string_type st = {t.str, false};
                std::vector<modoc::string_type> v;
                v.push_back(std::move(st));

                text_node* txt = new text_node(std::move(v));
                if (t.type == KEYWORD) {
                    txt->meta["color"] = {"#ca9ee6"};
                    txt->meta["font-style"] = {"italic"};
                }
                else if (t.type == TYPE) txt->meta["color"] = {"#e5c890"};
                
                last->add_node(txt);
            }
        }

        return result;*/
        return {};
    }


    ~new_code_node() override = default;
};

struct new_code_f : node_factory {
    node* instance(uint8_t, const options_t& op) override {
        return new new_code_node((std::string)op.at("lang").string());
    }

    void set_node_type_id(uint32_t id) const override {
        new_code_node::t_id = id;
    }
};
