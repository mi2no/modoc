#pragma once

#include <string_view>
#include <cstdint>
#include <vector>
#include <variant>
#include <stack>
#include <cstdio>
#include <list>

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
        return {start + 1, ptr};
    }

    struct uninitialized_tree {
        
        struct unode {
            struct node_type {
                std::string_view node_name;
                std::string_view tags, options, meta;
                std::vector<unode> children;
            };

            using value_type = std::variant<
                node_type,
                std::string_view,
                bool
            >;

            value_type _value;

            unode(bool) : _value(true) {}
            unode(std::string_view view) : _value(view) {}
            unode(std::string_view name, std::string_view tags, std::string_view options, std::string_view meta) : _value(node_type{name, tags, options, meta, {}}) {}

            bool is_node() const {
                return _value.index() == 0;
            }

            bool is_insert() const {
                return _value.index() == 2;
            }

            node_type& node() {
                return std::get<node_type>(_value);
            }

            const node_type& node() const {
                return std::get<node_type>(_value);
            }

            std::string_view text() const {
                return std::get<std::string_view>(_value);
            }

            void print() const {
                if (is_node()) {
                    const node_type& n = std::get<node_type>(_value);
                    printf("[?%.*s]", (int)n.node_name.size(), n.node_name.data());
                    if (n.tags.size()) printf("(%.*s)", (int)n.tags.size(), n.tags.data());
                    if (n.options.size()) printf("[%.*s]", (int)n.options.size(), n.options.data());
                    if (n.meta.size()) printf("{%.*s}", (int)n.meta.size(), n.meta.data());
                    putchar('\n');
                }
                else puts("[text]");
            }
        };

        std::vector<unode> nodes;

        static uninitialized_tree parse_document(const char* buffer) {
            uninitialized_tree result;

            size_t begin = 0, end = 0;
            bool word = false;

            std::stack<unode*> stack;
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

                        ++i; // skip KEYWORD_CHAR
                        const char* name_begin = buffer + i;
                        while (buffer[i] > ' ' && buffer[i] != '(' && buffer[i] != '[' && buffer[i] != '{') ++i;
                        const std::string_view name = {name_begin, buffer + i};
                        std::string_view tags, options, meta;

                        printf("%.*s %zu\n", (int)name.size(), name.data(), stack.size());

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

                        unode n = {name, tags, options, meta};

                        if (stack.size()) {
                            stack.top()->node().children.push_back(std::move(n));
                            stack.push(&stack.top()->node().children.back());
                        }
                        else {
                            result.nodes.push_back(std::move(n));
                            stack.push(&result.nodes.back());
                        }

                        --i;
                    }
                    else if (buffer[i] > ' ' && text_begin == nullptr) text_begin = buffer + i;

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

        static void print_node(const unode* n, std::list<bool>& branch_end, bool is_list_elm, size_t nest = 0) {
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
                const std::vector<unode>& children = n->node().children;

                branch_end.push_back(false);
                for (size_t i = 0; i < children.size(); ++i) {
                    branch_end.back() = (i == children.size() - 1);
                    print_node(&children[i], branch_end, n->node().node_name == "list", nest);
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
}
