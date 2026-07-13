#pragma once

#include <string>
#include <vector>
#include <list>
#include <cstdint>

#include "node.hpp"
#include "string_type.hpp"
#include "value.hpp"

namespace modoc {

    constexpr char KEYWORD_CHAR = '@';
    constexpr char EVALUATE_CHAR = '$';

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

    static bool primitives_only = true;

    static std::vector<node*> create_tree(const char* const& buffer, const uint8_t init_depth = 0) {
        std::vector<node*> result;
        
        size_t begin = 0, end = 0;
        bool word = false;
        //std::stack<keyword_instance*> stack;
        std::stack<node*> stack;
        std::vector<modoc::string_type> tokens;
        std::map<std::string_view, value> meta;

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
                if ((stack.empty() || !stack.top()->verbatim()) &&  buffer[i] == EVALUATE_CHAR) {
                    ++i; // Skip EVALUATE_CHAR
                    const char* end = buffer + i;
                    const std::string result = evaluate(end, &end);
                    
                    tokens.emplace_back(result, true);
                    i = end - buffer;
                }
                else if (word && buffer[begin] == KEYWORD_CHAR) {
                    if (buffer[i] == '[') {
                        end = i;
                        size_t x = parse_options(buffer + i, options);//get_options(buffer + i + 1, options);
                        i += x;
                        printf("Read %zu\n", x);
                    }
                    /*else if (buffer[i] == '{') {
                        const size_t read = parse_options(buffer + i, meta, '}');
                        i += read;
                    }*/
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
                            std::string_view view{buffer + begin + 1, end - begin - 1};

                            if (nodes.contains(view)) {
                                if (tokens.size()) {
                                    //if (stack.size()) result += stack.top()->format(tokens, dependecies);
                                    //else put_tokens(result, tokens);
                                    if (stack.size()) stack.top()->parse_tokens(std::move(tokens), (stack.size() < tabs) * (tabs - stack.size()));
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
                                    node* instance = nodes[view]->instance(tabs + init_depth, options);
                                    options.clear();

                                    if (meta.size()) {
                                        instance->add_meta(meta);
                                        meta.clear();
                                    }

                                    if (!instance->is_primitive()) primitives_only = false;
                                    
                                    if (stack.size()) stack.top()->add_node(instance);
                                    else result.push_back(instance);
                                    
                                    stack.push(instance);

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
                    stack.top()->parse_tokens(std::move(tokens), (stack.size() < tabs) * (tabs - stack.size()));
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
                stack.top()->parse_tokens(std::move(tokens), 0);
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

        auto itr = tree.begin();
        while (itr != tree.end()) {
            node* n = *itr;
            n->debug_print();

            if (!n->is_primitive()) {
                std::vector<node*> primitives = ((special_node*)n)->expand();
                itr = tree.erase(itr);

                printf("expanded size: %zu\n", primitives.size());

                if (primitives.size()) itr = tree.insert(itr, primitives.begin(), primitives.end());
                //--itr; // In the next loop the first new "primitive" will be tested
                continue;
            }
            else if (n->child_nodes() != nullptr) to_primitive_tree(*(std::vector<node*>*)n->child_nodes());

            ++itr;
        }
    }
};
