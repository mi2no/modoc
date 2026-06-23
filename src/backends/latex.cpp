#include <cstdio>
#include <string_view>
#include <unordered_set>
#include <vector>
#include <stack>

#include "../node.hpp"

std::unordered_set<std::string> dependencies;

std::string node_to_str(const node* n) {
    std::string result;

    //printf("%u %u\n", sec_node::t_id, list_node::t_id);

    //printf("%u ", n->type_id());

    //if (n->type_id() == sec_node::t_id) {
    if (node::is_type<sec_node>(n)) {
        //n->debug_print();
        const sec_node* s = (const sec_node*)n;
        
        result += '\\';

        for (uint8_t i = 1; i < s->id_size; ++i) result += "sub";

        result += "section{";
        result += s->title;
        result += "}\n";

        const std::vector<node*>* children = n->child_nodes();

        if (children != nullptr)
            for (const node* ch : *children)
                result += node_to_str(ch);
    }
    else if (n->type_id() == list_node::t_id) {
        list_node* l = (list_node*)n;
        result += "\\begin{itemize}\n";
 
        const std::vector<node*>* children = n->child_nodes();

        if (children != nullptr)
            for (const node* ch : *children) {
                result += "\\item ";
                result += node_to_str(ch);
            }

        result += "\\end{itemize}\n";
    }
    else if (node::is_type<code_node>(n)) {
        code_node* c = (code_node*)n;
        result += "\\begin{lstlisting}\n";
        bool line = false;

        for (code_node::token_t t : c->tokens) {
            if (t.type == code_node::NEWL) {
                if (line) result += '\n';
                for (uint8_t i = 0; i < (uint8_t)t.str[0]; ++i)
                    result += (char)9;
                line = true;
            }
            else {
                result += t.str;
                result += ' ';
            }
        }
        result += "\\end{lstlisting}";
    }
    else {
        text_node* t = (text_node*)n;

        for (std::string_view sv : t->tokens) {
            result += sv;
            result += ' ';
        }
        result += '\n';
    }


    return result;
}

extern "C" void compile(const std::vector<node*>& tree) {
    /*struct node_info {
        size_t id;
        node* parent;
    };
    std::stack<node_info> stack;
    stack.push({0, nullptr});

    while (stack.size()) {
        const node* n;
        const node_info& top = stack.top();

        if (top.parent == nullptr) n = tree[top.id];
        else n = (*top.parent->child_nodes())[top.id];

        ++top.id;
        if ()

        print_node(n);
    }*/

    FILE* const out = fopen("out.tex", "w");

    std::string result;

    result += "\\documentclass{article}\n\n\\begin{document}\n";

    for (const node* n : tree) {
        result += node_to_str(n);
    }

    result += "\\end{document}";

    fputs(result.c_str(), out);

    fclose(out);
}
