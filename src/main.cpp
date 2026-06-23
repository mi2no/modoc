#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cstdint>
#include <list>
#include <stack>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "node.hpp"
#include "options.hpp"

constexpr char KEYWORD_CHAR = '@';

//std::unordered_map<std::string_view, keyword*> keywords;
std::map<std::string_view, object> options;
std::unordered_set<std::string> dependecies;

/*void handle_math(const char* const& buffer, size_t& i, std::string& s) {
    size_t begin = ++i, p_begin = 0, p_end = 0;
    s += '$';
    while (buffer[i] != '$' && buffer[i] != '\0') {
        if (buffer[i] == '{') p_begin = i;
        else if (buffer[i] == '}') p_end = i;
        else if (buffer[i] == '/') {
            //s.append(buffer + begin, i - begin - 1);
            s += "\\frac{";
            if (p_end > p_begin) {
                s.append(buffer + p_begin + 1, p_end - p_begin - 1);
                p_end = p_begin;
            }
            else s += buffer[i - 1];
            s += "}{";
            s += buffer[++i];
            s += '}';
        }
        else if (p_end == p_begin) s += buffer[i];
        ++i;
    }
    s += '$';
}*/

std::unordered_map<std::string_view, node_factory*> nodes;

void put_tokens(std::string& s, const std::vector<std::string_view>& tokens, const uint8_t& nest = 0) {
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

std::vector<node*> create_tree(const char* const& buffer) {
    std::vector<node*> result;
    
    size_t begin = 0, end = 0;
    bool word = false;
    //std::stack<keyword_instance*> stack;
    std::stack<node*> stack;
    std::vector<std::string_view> tokens;

    for (size_t i = 0; buffer[i] != '\0'; ++i) {
        size_t tabs = 0;
        while (buffer[i] == '\t') {
            ++tabs;
            ++i;
        }
        while (tabs < stack.size()) {
            if (tokens.size()) {
                //result += stack.top()->format(tokens, dependecies);
                stack.top()->parse_tokens(tokens, 0);

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
            if (word && buffer[begin] == KEYWORD_CHAR && buffer[i] == '[') {
                end = i;
                size_t x = get_options(buffer + i + 1, options);
                i += x;
                printf("Read %zu\n", x);
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
                                if (stack.size()) stack.top()->parse_tokens(tokens, (stack.size() < tabs) * (tabs - stack.size()));
                                else result.push_back(new text_node(tokens));

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
                                node* instance = nodes[view]->instance(tabs, options);
                                options.clear();
                                
                                if (stack.size()) stack.top()->add_node(instance);
                                else result.push_back(instance);
                                
                                stack.push(instance);

                                //for (size_t n = 0; n < tabs; ++n) result += '\t';
                                //result += ik->begin;
                            }
                        }
                        //else result.append(buffer + begin, end - begin);
                        else tokens.push_back({buffer + begin, end - begin});
                    }
                    else {
                        tokens.push_back({buffer + begin, end - begin});
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
                stack.top()->parse_tokens(tokens, (stack.size() < tabs) * (tabs - stack.size()));
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
            stack.top()->parse_tokens(tokens, 0);
            tokens.clear();
        }

        //result += stack.top()->end;
        //result += '\n';
        //delete stack.top();
        stack.pop();
    }

    //put_tokens(result, tokens);
    if (tokens.size()) result.push_back(new text_node(tokens));

    return result;
}

void print_node(const node* n, std::list<uint8_t>& sec_id, std::list<bool>& branch_end, bool is_list_elm, size_t nest = 0) {
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

void print_doc_tree(const std::vector<node*> tree) {
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

#include <dlfcn.h>

void to_doc(const std::vector<node*>& tree, const char* backend_path) {
    void* handle = dlopen(backend_path, RTLD_LAZY);

    if (!handle) {
        fputs("Error 1\n", stderr);
    }

    typedef void(*func_t)(const std::vector<node*>&);

    func_t f_handle = (func_t)dlsym(handle, "compile");

    if (!f_handle) {
        fputs("Error 2\n", stderr);
    }

    f_handle(tree);
}

void register_node_factory(const char* sym, node_factory* nf) {
    static uint32_t type_id_itr = 1; // 0 - text_node
    
    nf->set_node_type_id(type_id_itr++);
    nodes[sym] = nf;
}

void destroy_node(node* n) {
    const std::vector<node*>* children = n->child_nodes();

    if (children != nullptr)
        for (node* ch : *children)
            destroy_node(ch);

    delete n;
}

void destroy_tree(std::vector<node*>& tree) {
    for (node* n : tree)
        destroy_node(n);
    tree.clear();
}

#include <filesystem>
#include <stdlib.h>
#include <libgen.h>

void get_backends(std::vector<std::string>& v) {
    /*char* const path = dirname(realpath("/proc/self/exe", nullptr));
    const size_t len = strlen(path);

    char* const joined = (char*)malloc(sizeof(char) * (len + 10));
    memcpy(joined, path, len);
    free(path);
    memcpy(joined + len, "/backends", 9);
    joined[len + 9] = '\0';

    puts(joined);*/

    std::filesystem::directory_iterator itr("backends");//joined);

    //free(joined);
    
    for (auto entry : itr) {
        std::string path = entry.path();
        const size_t s = path.size();

        if (path[s - 3] == '.' && path[s - 2] == 's' && path[s - 1] == 'o')
            v.push_back(path);
    }
}

bool str_match(const char* a, const char* b) {
    while (*a != '\0' || *b != '\0')
        if (*a++ != *b++) return false;
    return true;
}


int main(int argc, char** argv) {

    // ./main [src_file] --backend
    
    std::vector<std::string> backends;
    get_backends(backends);
    
    if (argc < 3 || (argv[2][0] != '-' || argv[2][1] != '-')) {
        printf("Usage: %s [src_file] --[backend]\n", argv[0]);
        puts("Available backends:");
        for (std::string& s : backends) {
            s[s.size() - 3] = '\0';
            //printf("- %s\n", s.c_str() + 11);
            printf("- %s\n", s.data());
        }
        return 0;
    }

    size_t backend_id = backends.size();

    for (size_t i = 0; i < backends.size(); ++i) {
        std::string& s = backends[i];

        s[s.size() - 3] = '\0';
        const bool match = str_match(s.c_str() + 9, argv[2] + 2);
        s[s.size() - 3] = '.';

        if (match) {
            backend_id = i; 
            break;
        }
    }

    if (backend_id == backends.size()) {
        puts("Backend not found. Available backends:");
        for (std::string& s : backends) {
            s[s.size() - 3] = '\0';
            //printf("- %s\n", s.c_str() + 11);
            printf("- %s\n", basename(s.data()));
        }
        return 0;
    }

    register_node_factory("sec", new sec_f());
    register_node_factory("list", new list_f());
    register_node_factory("code", new code_f());

    FILE* file = fopen(argv[1], "r");
    fseek(file, SEEK_SET, SEEK_END);
    size_t size = ftell(file);
    rewind(file);
    char* buffer = (char*)malloc(size);
    fread(buffer, 1, size, file);
   
    /*std::string result = inner_document(buffer);
    std::string header;
    for (const std::string& s : dependecies) {
        header += "\\usepackage{";
        header += s;
        header += "}\n";
    }
    result = "\\documentclass{article}\n\\usepackage[left=2.5cm,top=2.5cm,right=2.5cm,bottom=2.5cm]{geometry}\n" + header + result;
    puts(result.c_str());*/

    std::vector<node*> tree = create_tree(buffer);

    puts("Document tree structure:\n");
    print_doc_tree(tree);

    to_doc(tree, backends[backend_id].c_str());

    printf("\nCompiled via %s backend. (%s)\n", argv[2] + 2, backends[backend_id].c_str());

    destroy_tree(tree);
    free(buffer);

    for (const auto ent : nodes) {
        delete ent.second;
    }

    return 0;
}
