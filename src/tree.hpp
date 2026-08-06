#pragma once

#include <iterator>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <list>
#include <cstdint>
#include <stack>

#include "string_type.hpp"
#include "value.hpp"

#include "uninitialized_tree.hpp"

struct node;

namespace modoc {
    //static std::map<std::string_view, object> options;
    static options_t options;

    static void put_tokens(std::string& s, const std::vector<std::string_view>& tokens, const uint8_t& nest = 0) {
        for (size_t i = 0; i < tokens.size(); ++i) {
                const std::string_view& view = tokens[i];

                if (view.begin() == nullptr) {
                    s += '\n';
                    if (i < tokens.size() - 1) // nesting
                        for (uint8_t n = 0; n < nest; ++n) s += '\t';
                }
                else {
                    s += view;
                    s += ' ';
                }
            }
    }

    static std::vector<modoc::string_type> tokenize(std::string_view str, std::function<const value*(std::string_view)> get_variable = nullptr, bool copy = false) { // TODO: add evaluation (EVALUATE_CHAR)
        std::vector<modoc::string_type> tokens;
        const char *token = nullptr;

        for (auto itr = str.begin(); itr < str.end(); ++itr) {
            if (token != nullptr && (*itr == ' ' || *itr == '\t' || *itr == '\n')) {
                tokens.emplace_back(std::string_view{token, itr}, copy);
                token = nullptr;
            }
            else if (token == nullptr) {
                if (*itr == EVALUATE_CHAR) {
                    tokens.emplace_back(evaluate(itr + 1, &itr, get_variable), true);
                    //printf("off: %zu\nend: %d\n", str.end() - itr, (int)*str.end());
                }
                else if (*itr != ' ' && *itr != '\t' && *itr != '\n') token = itr;   
            }
        }

        if (token != nullptr) tokens.emplace_back(std::string_view{token, str.end()}, copy);

        /*for (auto& t : tokens) {
            const std::string_view view = t.view();
            printf("\"%.*s\"\n", (int)view.size(), view.data());
        }
        fflush(stdout);*/

        return tokens;
    }

    /*static void apply_meta(const std::vector<node*>& tree, const options_t& meta) {
        for (node* n : tree) {
            for (const auto& entry : meta)
                if (!n->meta.contains(entry.first))
                    n->meta[entry.first] = entry.second;

            const std::vector<node*>* children = n->child_nodes();
            if (children != nullptr) apply_meta(*children, meta);
        }
    }*/

    static bool primitives_only = true;

    /*static void print_node(const node* n, std::list<uint8_t>& sec_id, std::list<bool>& branch_end, bool is_list_elm, size_t nest = 0) {
        //putchar('+');

        for (std::list<bool>::iterator itr = branch_end.begin(); itr != --branch_end.end(); ++itr) {
            if (!*itr) fputs("\u2502  ", stdout);
            else fputs("   ", stdout);
        }

        if (!branch_end.back()) fputs("\u251C", stdout);
        else fputs("\u2514", stdout); 

        if (is_list_elm) fputs("\u2500\u25A1", stdout); // \u25CF - full circle  \u25EF - circle
        else fputs("\u2500\u2500", stdout);*/

        /*if (debug_str_match(n->type(), "section")) {
            ++sec_id.back();
            for (uint8_t i : sec_id)
                printf("%hhu.", i);
            sec_id.push_back(0);
        }*/

        /*printf("[%u]", n->type_id());
        n->debug_print();
        ++nest;

        const std::vector<node*>* children = n->child_nodes();
        
        if (children != nullptr) {
            branch_end.push_back(false);
            for (size_t i = 0; i < children->size(); ++i) {
                branch_end.back() = (i == children->size() - 1);
                print_node((*children)[i], sec_id, branch_end, debug_str_match(n->type(), "list"), nest);
            }
            branch_end.pop_back();
        }

        //if (debug_str_match(n->type(), "section")) sec_id.pop_back();

        //delete n;
    }*/

    /*static void print_doc_tree(const std::vector<node*>& tree) {
        //puts("untitled");
        puts("\u25CF");
        std::list<uint8_t> sec_id;
        std::list<bool> branch_end;
        branch_end.push_back(false);

        sec_id.push_back(0);

        for (size_t i = 0; i < tree.size(); ++i) {
            branch_end.back() = i == tree.size() - 1;
            print_node(tree[i], sec_id, branch_end, false);
        }
    }

    static void to_primitive_tree(std::vector<node*>& tree) {
        if (primitives_only) return;

        puts("here");

        bool second_pass = false;
        for (uint8_t pass = 0; pass < 2; ++pass) {
            auto itr = tree.begin();
            while (itr != tree.end()) {
                node* n = *itr;
                n->debug_print();

                if (!n->is_primitive()) {
                    special_node* sn = (special_node*)n;

                    if (pass == 0 && sn->in_second_pass()) second_pass = true;
                    else {
                        std::vector<node*> primitives = sn->expand(tree);
                        // TODO: delete
                        itr = tree.erase(itr);

                        printf("expanded size: %zu\n", primitives.size());

                        if (primitives.size()) itr = tree.insert(itr, primitives.begin(), primitives.end());
                        //--itr; // In the next loop the first new "primitive" will be tested
                        continue;
                    }
                }
                else if (n->child_nodes() != nullptr) to_primitive_tree(*(std::vector<node*>*)n->child_nodes());

                ++itr;
            }
            if (!second_pass) break;
        }
    }

    static void tree_final_pass(const std::vector<node*>& core_tree) {
        for (node* n : core_tree) {
            n->final_pass();
            if (n->child_nodes() != nullptr) tree_final_pass(*n->child_nodes());
        }
    }*/

    struct tree {

        struct layered_var {
            value v;//value;
            uint32_t begin_ind;
        };

        std::vector<node*> nodes;
        std::map</*modoc::string_type*/std::string, std::stack<layered_var>> variables;
        //uint32_t ind = 0;

    private:

        static std::vector<node*> initialize_node(tree& tree, uninitialized_tree::unode& un, const uint8_t depth, const bool copy_text = false); 
        static tree initialize(std::vector<uninitialized_tree::unode>& unodes, const uint8_t depth = 0, const bool copy_text = false);

        void print_node(const node* n, std::list<bool>& branch_end, bool is_list_elm, size_t nest = 0) const;
        std::string node_to_str(const node* n, std::list<bool>& branch_end, bool is_list_elm, size_t nest = 0) const;


        void _assign_at(std::string_view _name, value&& v, uint32_t ind) {
            std::string name = std::string(_name);
            
            //printf("[tree] %s : %s\n", name.c_str(), v.to_string().c_str());
            if (variables.contains(name)) {
                auto& stack = variables.at(name);
                if (stack.top().begin_ind == ind) stack.top().v/*alue*/ = std::move(v);
                else stack.push({std::move(v), ind});
            }
            else {
                auto& stack = variables[name] = {};
                variables[name].push({std::move(v), ind});
            }
            //printf("Variables: %zu\n", variables.size());
        }


    public:

        tree() = default;

        tree(tree&& t) {
            nodes = std::move(t.nodes);
            variables = std::move(t.variables);
            //ind = t.ind;

            t.nodes.clear();
            t.variables.clear();
        }

        static tree initialize(uninitialized_tree u_tree) {
            return initialize(u_tree.nodes, 0);
        }

        void assign(std::string_view name, value&& v) {
            _assign_at(name, std::move(v), nodes.size());
        }

        const value* get_variable(std::string_view _name) const {
            std::string name = std::string(_name);
            
            if (variables.contains(name)) return &variables.at(name).top().v;
            else return nullptr;
        }

        void print() const {
            puts("\u25CF");
            std::list<bool> branch_end;
            branch_end.push_back(false);

            for (size_t i = 0; i < nodes.size(); ++i) {
                branch_end.back() = i == nodes.size() - 1;
                print_node(nodes[i], branch_end, false);
            }

            printf("Variables: %zu\n", variables.size());
            for (const auto& entry : variables) {
                auto stack = entry.second;
                printf("$%s:\n", entry.first.c_str());
                
                while (stack.size()) {
                    printf("\t%u : %s\n", stack.top().begin_ind, stack.top().v.to_string().c_str());
                    stack.pop();
                }
            }
        }
        
        std::string to_string() const {
            std::string result = "\u25CF\n";
            std::list<bool> branch_end;
            branch_end.push_back(false);

            for (size_t i = 0; i < nodes.size(); ++i) {
                branch_end.back() = i == nodes.size() - 1;
                result += node_to_str(nodes[i], branch_end, false);
            }

            /*printf("Variables: %zu\n", variables.size());
            for (const auto& entry : variables) {
                auto stack = entry.second;
                printf("$%s:\n", entry.first.c_str());
                
                while (stack.size()) {
                    printf("\t%u : %s\n", stack.top().begin_ind, stack.top().v.to_string().c_str());
                    stack.pop();
                }
            }*/

            return result;
        }

        void destroy_node(node* n);

        void destroy_tree(std::vector<node*>& tree) {
            for (node* n : tree)
                destroy_node(n);
            tree.clear();
        }

        ~tree() {
            //print();
            fflush(stdout);
            destroy_tree(nodes);
        }
    };
   
}
