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
        
        result += "<div class=\"sec\" title=\"";
        result += s->title;
        result += "\" num=\"";
        result += std::to_string(s->id[s->id_size - 1]);
        result += "\">";

        const std::vector<node*>* children = n->child_nodes();

        if (children != nullptr)
            for (const node* ch : *children)
                result += node_to_str(ch);

        result += "</div>";
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

    result += "<style> \n\
    @import url('https://fonts.googleapis.com/css2?family=Funnel+Display:wght@300..800&family=Google+Sans+Flex:opsz,wght@6..144,1..1000&family=JetBrains+Mono:ital,wght@0,100..800;1,100..800&family=Momo+Trust+Display&family=Open+Sans:ital,wght@0,300..800;1,300..800&family=Playfair+Display:ital,wght@0,400..900;1,400..900&family=Poppins:ital,wght@0,100;0,200;0,300;0,400;0,500;0,600;0,700;0,800;0,900;1,100;1,200;1,300;1,400;1,500;1,600;1,700;1,800;1,900&family=Roboto+Slab:wght@100..900&display=swap');\n\
    body { \n\
        font-family: 'Roboto Slab', serif; \n\
    } \n\
    .sec { \n\
        --heading-size: 2rem; \n\
        --sec-color: lightgray; \n\
        display: flex; \n\
        border-width: 0 0 0 2px; \n\
        border-color: var(--sec-color); \n\
        border-style: solid; \n\
        margin-top: var(--heading-size); \n\
        #margin-left: 10px; \n\
        margin-left: calc(var(--heading-size) / 2); \n\
        padding: 20px 0 20px 20px; \n\
        text-align: justify; \n\
        flex-direction: column; \n\
        gap: 20px; \n\
    } \n\
    .sec:before { \n\
        order: 0; \n\
        position: absolute; \n\
        content: attr(title); \n\
        margin-top: calc(-1em - 20px);#calc(var(--heading-size) * -1); \n\
        margin-left: calc(1em - 20px); \n\
        font-size: var(--heading-size); \n\
        font-weight: bold; \n\
        font-family: 'Funnel Display', sans-serif\n\
    } \n\
    .sec:after { \n\
        display: block; \n\
        order: 1; \n\
        position: absolute; \n\
        content: attr(num); \n\
        #margin-left: -20px; \n\
        margin-left: calc(var(--heading-size) / -2 - 20px); \n\
        margin-top: calc(var(--heading-size) * -1 - 20px); \n\
        aspect-ratio: 1; \n\
        width: var(--heading-size); \n\
        background-color: var(--sec-color); \n\
        border-radius: 50%; \n\
        align-content: center; \n\
        text-align: center; \n\
        font-weight: bold;\n\
        font-family: 'Google Sans Flex', sans-serif;\n\
        color: #5E5E5E;\n\
    } \n\
    </style>";

    for (const node* n : tree) {
        result += node_to_str(n);
    }

    result += "</html>";

    fputs(result.c_str(), out);

    fclose(out);
}
