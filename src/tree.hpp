#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <list>
#include <cstdint>

#include "node.hpp"
#include "string_type.hpp"
#include "value.hpp"

namespace modoc {

    constexpr char KEYWORD_CHAR = '@';
    constexpr char EVALUATE_CHAR = '$';


    static std::string_view get_scope(const char* ptr, const char begin, const char end) {
        const char* start = ptr;
        {
            uint8_t depth = 1;

            while (depth) {
                ++ptr;
                if (*ptr == begin) ++depth;
                else if (*ptr == end) --depth;
            }
        } 
        return {start, ptr};
    }

    struct uninitialized_tree {
        
        struct uninitialized_node : group_node {
            struct node_type {
                std::string_view node_name;
                std::string_view tags, options, meta;
                std::vector<uninitialized_node> children;
            };

            using value_type = std::variant<
                node_type,
                std::string_view
            >;

            value_type _value;

            uninitialized_node(std::string_view view) : _value(view) {}
            uninitialized_node(std::string_view name, std::string_view tags, std::string_view options, std::string_view meta) : _value(node_type{name, tags, options, meta, {}}) {}

            bool is_node() const {
                return _value.index() == 0;
            }

            node_type& node() {
                return std::get<node_type>(_value);
            }

            const node_type& node() const {
                return std::get<node_type>(_value);
            }

            void print() const {
                if (is_node()) {
                    const node_type& n = std::get<node_type>(_value);
                    printf("[?%.*s]\n", (int)n.node_name.size(), n.node_name.data());
                }
                else puts("[text]");
            }
        };

        std::vector<uninitialized_node> nodes;

        static uninitialized_tree parse_document(const char* buffer) {
            uninitialized_tree result;

            size_t begin = 0, end = 0;
            bool word = false;

            std::stack<uninitialized_node*> stack;
            const char* text_begin = nullptr;

            size_t i = 0;
            for (; buffer[i] != '\0'; ++i) {
                size_t tabs = 0;
                while (buffer[i] == '\t') {
                    ++tabs;
                    ++i;
                }
                while (tabs < stack.size()) {
                    if (text_begin != nullptr) {
                        stack.top()->node().children.emplace_back(std::string_view{text_begin, buffer + i - tabs});
                        text_begin = nullptr;
                    }

                    //for (size_t n = 0; n < tabs; ++n) result += '\t';
                    //result += stack.top()->end;
                    //result += '\n';
                    //delete stack.top();
                    stack.pop();
                }

                // Line
                while (buffer[i] != '\n' && buffer[i] != '\0') {
                    if (buffer[i] == KEYWORD_CHAR) {
                        if (text_begin != nullptr) {
                            if (stack.size()) stack.top()->node().children.emplace_back(std::string_view{text_begin, buffer + i - tabs});
                            else result.nodes.emplace_back(std::string_view{text_begin, buffer + i - tabs});
                            text_begin = nullptr;
                        }

                        const char* name_begin = buffer + i;
                        while (buffer[i] > ' ' && buffer[i] != '(' && buffer[i] != '[' && buffer[i] != '{') ++i;
                        const std::string_view name = {name_begin, buffer + i};
                        std::string_view tags, options, meta;

                        while (buffer[i] == '(' || buffer[i] == '[' || buffer[i] == '{') {
                            if (buffer[i] == '(') {
                                tags = get_scope(buffer + i, '(', ')');
                                i += tags.size() + 2;
                            }
                            else if (buffer[i] == '[') {
                                options = get_scope(buffer + i, '[', ']');
                                i += options.size() + 2;
                            }
                            else if (buffer[i] == '{') {
                                meta = get_scope(buffer + i, '{', '}');
                                i += meta.size() + 2;
                            }
                        }

                        uninitialized_node n = {name, tags, options, meta};

                        if (stack.size()) {
                            stack.top()->node().children.push_back(std::move(n));
                            stack.push(&stack.top()->node().children.back());
                        }
                        else {
                            result.nodes.push_back(std::move(n));
                            stack.push(&result.nodes.back());
                        } 
                    }
                    
                    if (buffer[i] > ' ' && text_begin == nullptr) text_begin = buffer + i;

                    ++i;
                }
            }

            while (stack.size()) {
                if (text_begin != nullptr) {
                    stack.top()->node().children.emplace_back(std::string_view{text_begin, buffer + i});
                    text_begin = nullptr;
                }
                stack.pop();
            }

            if (text_begin != nullptr) result.nodes.emplace_back(std::string_view{text_begin, buffer + i});


            return result;
        }

        static void print_node(const uninitialized_node* n, std::list<bool>& branch_end, bool is_list_elm, size_t nest = 0) {
            //putchar('+');

            for (std::list<bool>::iterator itr = branch_end.begin(); itr != --branch_end.end(); ++itr) {
                if (!*itr) fputs("\u2502  ", stdout);
                else fputs("   ", stdout);
            }

            if (!branch_end.back()) fputs("\u251C", stdout);
            else fputs("\u2514", stdout); 

            if (is_list_elm) fputs("\u2500\u25A1", stdout); // \u25CF - full circle  \u25EF - circle
            else fputs("\u2500\u2500", stdout);

            n->print();
            ++nest;

            
            if (n->is_node() && n->node().children.size()) {
                const std::vector<uninitialized_node>& children = n->node().children;

                branch_end.push_back(false);
                for (size_t i = 0; i < children.size(); ++i) {
                    branch_end.back() = (i == children.size() - 1);
                    print_node(&children[i], branch_end, debug_str_match(n->type(), "list"), nest);
                }
                branch_end.pop_back();
            }

        }

        void print() {
            //puts("untitled");
            puts("\u25CF");
            std::list<bool> branch_end;
            branch_end.push_back(false);

            for (size_t i = 0; i < nodes.size(); ++i) {
                branch_end.back() = i == nodes.size() - 1;
                print_node(&nodes[i], branch_end, false);
            }
        }

    };

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

    static std::vector<modoc::string_type> tokenize(std::string_view str) { // TODO: add evaluation (EVALUATE_CHAR)
        std::vector<modoc::string_type> tokens;
        size_t token_begin;
        bool token = false;

        for (auto itr = str.begin(); itr != str.end(); ++itr) {
            if (token && (*itr == ' ' || *itr == '\t' || *itr == '\n')) {
                tokens.emplace_back(std::string_view{str.begin() + token_begin, itr}, false);
                token = false;
            }
            else if (!token) {
                token = true;
                token_begin = itr - str.begin();
            }
        }

        if (token) tokens.emplace_back(std::string_view{str.begin() + token_begin, str.end()}, false);

        return tokens;
    }

    static void apply_meta(const std::vector<node*>& tree, const options_t& meta) {
        for (node* n : tree) {
            for (const auto& entry : meta)
                if (!n->meta.contains(entry.first))
                    n->meta[entry.first] = entry.second;

            const std::vector<node*>* children = n->child_nodes();
            if (children != nullptr) apply_meta(*children, meta);
        }
    }

    static bool primitives_only = true;

    static std::vector<node*> create_tree(const char* const& buffer, const uint8_t init_depth = 0) {
        std::vector<node*> result;
        
        size_t begin = 0, end = 0;
        bool word = false;
        //std::stack<keyword_instance*> stack;
        std::stack<node*> stack;
        std::vector<modoc::string_type> tokens;
        //std::map<std::string_view, value> meta;

        std::string_view tags, options, meta;

        for (size_t i = 0; buffer[i] != '\0'; ++i) {
            size_t tabs = 0;
            while (buffer[i] == '\t') {
                ++tabs;
                ++i;
            }
            while (tabs < stack.size()) {
                if (tokens.size()) {
                    //result += stack.top()->format(tokens, dependecies);
                    stack.top()->parse_tokens(std::move(tokens), 0);

                    tokens.clear();
                }

                //for (size_t n = 0; n < tabs; ++n) result += '\t';
                //result += stack.top()->end;
                //result += '\n';
                //delete stack.top();
                stack.pop();
            }
            
            // Line
            while (buffer[i] != '\0') {
                /*if ((stack.empty() || !stack.top()->verbatim()) && buffer[i] == EVALUATE_CHAR) {
                    ++i; // Skip EVALUATE_CHAR
                    const char* end = buffer + i;
                    const std::string result = evaluate(end, &end);
                    
                    tokens.emplace_back(result, true);
                    i = end - buffer;
                }
                else*/ 
                if (word && buffer[begin] == KEYWORD_CHAR && buffer[i] > ' ') {
                    if (buffer[i] == '[') {
                        //if (node_name.data() == nullptr) node_name = {buffer + begin, buffer + i};
                        end = i;
                        options = get_scope(buffer + i, '[', ']');
                        i += options.size() + 1;
                    }
                    else if (buffer[i] == '{') {
                        //if (node_name.data() == nullptr) node_name = {buffer + begin, buffer + i};
                        end = i;
                        meta = get_scope(buffer + i, '{', '}');
                        i += options.size() + 1;
                    }
                }
                else if (buffer[i] > ' ' && !word) {
                    begin = i;
                    word = true;
                }
                else if (buffer[i] <= ' ') {
                    if (word) {
                        if (end <= begin) end = i;
                        word = false;

                        if (buffer[begin] == KEYWORD_CHAR) {
                            const std::string_view node_name = {buffer + begin + 1, end - begin - 1};

                            if (nodes.contains(node_name)) {
                                if (tokens.size()) {
                                    //if (stack.size()) result += stack.top()->format(tokens, dependecies);
                                    //else put_tokens(result, tokens);
                                    /////////if (stack.size()) stack.top()->parse_tokens(std::move(tokens), (stack.size() < tabs) * (tabs - stack.size()));
                                    if (stack.size()) stack.top()->add_node(new text_node(std::move(tokens)));
                                    else result.push_back(new text_node(std::move(tokens)));

                                    tokens.clear();
                                }

                                /*const keyword* k = keywords[view];
                                keyword_instance* ik = k->get_instance(tabs, options);
                                options.clear();*/

                                //printf("scope: %hhu\n", ik->scope_end());

                                /*if (ik->scope_end() == START) {
                                    result += ik->format(tokens, dependecies);
                                    delete ik;
                                }
                                else*/ {
                                    //stack.push(ik);
                                    //

                                    /*node* instance = nodes[view]->instance(tabs + init_depth, options);
                                    options.clear();

                                    if (meta.size()) {
                                        instance->add_meta(meta);
                                        meta.clear();
                                    }

                                    if (!instance->is_primitive()) primitives_only = false;*/

                                    /*uninitialized_node* instance = new uninitialized_node();
                                    instance->node_name = node_name;
                                    instance->tags = tags;
                                    instance->options = options;
                                    instance->meta = meta;
                                    
                                    if (stack.size()) stack.top()->add_node(instance);
                                    else result.push_back(instance);
                                    
                                    stack.push(instance);*/

                                    //for (size_t n = 0; n < tabs; ++n) result += '\t';
                                    //result += ik->begin;
                                }
                            }
                            //else result.append(buffer + begin, end - begin);
                            else tokens.push_back({{buffer + begin, end - begin}, false});
                        }
                        else {
                            tokens.push_back({{buffer + begin, end - begin}, false});
                            //result.append(buffer + begin, end - begin);
                        }
                    }
                    //result += buffer[i];
                    //TODO maybe remove this?
                    if (buffer[i] == '\n') {
                        //tokens.push_back({nullptr, 0});
                        break;
                    }
                }
                ++i;
            }
            
            if (stack.size()) {
                if (tokens.size()/* > 1*/) {
                    //tokens.pop_back(); // remove \n -> std::stringview{nullptr, 0} token
                    //result += stack.top()->format(tokens, dependecies);
                    ///////stack.top()->parse_tokens(std::move(tokens), (stack.size() < tabs) * (tabs - stack.size()));
                    stack.top()->add_node(new text_node(std::move(tokens)));
                    tokens.clear();
                }

                //result += stack.top()->end;
                //result += '\n';
                //delete stack.top();
                //stack.pop();
            }

            /*if (stack.size() && stack.top()->scope_end() == ENDL) {
                if (tokens.size() > 1) {
                    tokens.pop_back();
                    //result += stack.top()->format(tokens, dependecies);
                    stack.top()->nodes.push_back(new text_node(tokens));
                    tokens.clear();
                }

                //result += stack.top()->end;
                //result += '\n';
                //delete stack.top();
                stack.pop();  
            }*/
        }

        while (stack.size()) {
    if (tokens.size()) {
                //result += stack.top()->format(tokens, dependecies);
                ///////////////stack.top()->parse_tokens(std::move(tokens), 0);
                stack.top()->add_node(new text_node(std::move(tokens)));
                tokens.clear();
            }

            //result += stack.top()->end;
            //result += '\n';
            //delete stack.top();
            stack.pop();
        }

        //put_tokens(result, tokens);
        if (tokens.size()) result.push_back(new text_node(std::move(tokens)));

        return result;
    }

    static void print_node(const node* n, std::list<uint8_t>& sec_id, std::list<bool>& branch_end, bool is_list_elm, size_t nest = 0) {
        //putchar('+');

        for (std::list<bool>::iterator itr = branch_end.begin(); itr != --branch_end.end(); ++itr) {
            if (!*itr) fputs("\u2502  ", stdout);
            else fputs("   ", stdout);
        }

        if (!branch_end.back()) fputs("\u251C", stdout);
        else fputs("\u2514", stdout); 

        if (is_list_elm) fputs("\u2500\u25A1", stdout); // \u25CF - full circle  \u25EF - circle
        else fputs("\u2500\u2500", stdout);

        /*if (debug_str_match(n->type(), "section")) {
            ++sec_id.back();
            for (uint8_t i : sec_id)
                printf("%hhu.", i);
            sec_id.push_back(0);
        }*/

        printf("[%u]", n->type_id());
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
    }

    static void print_doc_tree(const std::vector<node*>& tree) {
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
    }
};
