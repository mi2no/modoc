#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "../node.hpp"

std::unordered_set<std::string> dependencies;

std::string node_to_str(const node* n) {
    std::string result;

    if (node::is_type<sec_node>(n)) {
        const sec_node* s = (const sec_node*)n;
        
        result += "<h";
        result += s->id_size + '0';
        result += '>';
        result += std::to_string(s->id[0]);

        for (uint8_t i = 1; i < s->id_size; ++i) {
            result += '.';
            result += std::to_string(s->id[i]);
        }

        result += ' ';
        result += s->title;
        result += "</h";
        result += s->id_size + '0';
        result += ">\n";

        const std::vector<node*>* children = n->child_nodes();

        if (children != nullptr)
            for (const node* ch : *children)
                result += node_to_str(ch);
    }
    else if (node::is_type<list_node>(n)) {
        list_node* l = (list_node*)n;
        result += "<ul>\n";
 
        const std::vector<node*>* children = n->child_nodes();

        if (children != nullptr)
            for (const node* ch : *children) {
                result += "<li>\n";
                result += node_to_str(ch);
                result += "</li>\n";
            }

        result += "</ul>\n";
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
    FILE* const out = fopen("out.html", "w");

    std::string result;

    result += "<!DOCTYPE HTML>\n<html>";

    for (const node* n : tree) {
        result += node_to_str(n);
    }

    result += "</html>";

    fputs(result.c_str(), out);

    fclose(out);
}
