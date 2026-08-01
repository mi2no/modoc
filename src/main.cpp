#include <algorithm>
#include <chrono>
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

#include "log.hpp"

#include "node.hpp"
//#include "options.hpp"
#include "value.hpp"
#include "tree.hpp"

// Nodes
#include "nodes/gen.hpp"
#include "nodes/repeat.hpp"
#include "nodes/meta.hpp"
#include "nodes/code.hpp"
#include "nodes/assign.hpp"
#include "nodes/if.hpp"


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


#include <dlfcn.h>

double to_doc(const modoc::tree& tree, const char* backend_path) {
    void* handle = dlopen(backend_path, RTLD_LAZY);

    if (!handle) {
        fputs("Error 1\n", stderr);
    }

    typedef void(*func_t)(const std::vector<node*>&);

    func_t f_handle = (func_t)dlsym(handle, "compile");

    if (!f_handle) {
        fputs("Error 2\n", stderr);
    }

    const auto start = std::chrono::high_resolution_clock::now();
    f_handle(tree.nodes);

    return (std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start)).count();
}

void register_node_factory(const char* sym, node_factory* nf) {
    static uint32_t type_id_itr = 1; // 0 - text_node
    
    nf->set_node_type_id(type_id_itr++);
    node_factories[sym] = nf;
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
    
    //{
        modoc::logger log;
    //}
    
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

    register_node_factory("group", new group_f());
    register_node_factory("sec", new sec_f());
    register_node_factory("list", new list_f());
    register_node_factory("code", new code_f());
    register_node_factory("gen", new gen_f());
    register_node_factory("repeat", new repeat_f());
    register_node_factory("if", new if_f());
    register_node_factory("meta", new meta_f());
    register_node_factory("assign", new assign_f());
    register_node_factory("new_code", new new_code_f());

    register_constant("code.lang.cpp", {"cpp"});
    register_constant("gen.mode.modoc", {gen_node::MODOC});
    register_constant("gen.mode.json", {gen_node::JSON});

    printf("Node factories: %zu\n", node_factories.size());

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

    /*std::vector<node*> tree = modoc::create_tree(buffer);

    puts("Document tree structure:\n");
    modoc::print_doc_tree(tree);*/

    modoc::uninitialized_tree un_tree = modoc::uninitialized_tree::parse_document(buffer);
    un_tree.print();

    modoc::tree tree = modoc::tree::initialize(std::move(un_tree));
    tree.print();

    log.width = 60;
    log.log("modoc", "core tree", tree.to_string());

    /*if (!modoc::primitives_only) {
        puts("To primitives:\n");
        modoc::to_primitive_tree(tree);
        modoc::print_doc_tree(tree);
    }

    modoc::tree_final_pass(tree);
    modoc::print_doc_tree(tree);*/

    const double time = to_doc(tree, backends[backend_id].c_str());

    log.log("modoc", "summary", std::string("Compiled via ") + (argv[2] + 2) + " backend.\nTime: " + std::to_string(time) + "s");

    //destroy_tree(tree);
    free(buffer);

    for (const auto ent : node_factories) {
        delete ent.second;
    }

    return 0;
}
