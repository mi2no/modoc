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
        //result += "<ul>\n";
        result += "<div class=\"list\">\n";
 
        const std::vector<node*>* children = n->child_nodes();

        if (children != nullptr)
            for (const node* ch : *children) {
                //result += "<li>\n";
                result += "<div>\n";
                result += node_to_str(ch);
                result += "</div>\n";
                //result += "</li>\n";
            }

        //result += "</ul>\n";
        result += "</div>\n";
    }
    else if (node::is_type<code_node>(n)) {
        code_node* c = (code_node*)n;
        bool line = false;

        result += "<div class=\"code\" lang=\"";
        result += c->lang;
        result += "\">\n";

        for (code_node::token_t t : c->tokens) {
            if (t.type == code_node::NEWL) {
                if (line) result += "</div>\n";
                result += "<div class=\"line\" style=\"--tabs: ";
                result += std::to_string((uint8_t)t.str[0]);
                result += ";\">\n";
                line = true;
            }
            else {
                result += "<span class=\"";

                switch (t.type) {
                    case code_node::TYPE:
                        result += "type";
                        break;
                    case code_node::KEYWORD:
                        result += "keyword";
                        break;
                }

                result += "\">";
                result += t.str;
                result += "</span>";
            }
        }

        result += "</div>\n</div>\n";
    }
    else if (node::is_type<text_node>(n)) {
        text_node* t = (text_node*)n;

        for (const modoc::string_type& s : t->tokens) {
            result += s.view();
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
    @import url('https://fonts.googleapis.com/css2?family=DM+Serif+Text:ital@0;1&family=Funnel+Display:wght@300..800&family=Google+Sans+Flex:opsz,wght@6..144,1..1000&family=JetBrains+Mono:ital,wght@0,100..800;1,100..800&family=Momo+Trust+Display&family=Open+Sans:ital,wght@0,300..800;1,300..800&family=Playfair+Display:ital,wght@0,400..900;1,400..900&family=Poppins:ital,wght@0,100;0,200;0,300;0,400;0,500;0,600;0,700;0,800;0,900;1,100;1,200;1,300;1,400;1,500;1,600;1,700;1,800;1,900&family=Roboto+Slab:wght@100..900&family=Source+Serif+4:ital,opsz,wght@0,8..60,200..900;1,8..60,200..900&display=swap');\n\
    body { \n\
        font-family: 'Source Serif 4', serif;\n\
        font-size: 1.2em;\n\
        padding: 10px;\n\
        font-weight: 370;\n\
        background-color: #1f1f1f;\n\
        color: white;\n\
    } \n\
    .sec { \n\
        --heading-size: 2rem; \n\
        --sec-color: #5e5e5e; \n\
        display: flex; \n\
        border-width: 0 0 0 1px; \n\
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
        font-size: var(--heading-size); \n\
        font-family: 'DM Serif Text', serif;\n\
    } \n\
    .sec:after { \n\
        display: block; \n\
        order: 1; \n\
        position: absolute; \n\
        content: attr(num); \n\
        #margin-left: -20px; \n\
        margin-left: calc(var(--heading-size) / sqrt(2) / -2 - 21.5px); \n\
        margin-top: calc(var(--heading-size) * -1 - 21px + (var(--heading-size) - var(--heading-size) / sqrt(2)) / 2); \n\
        aspect-ratio: 1; \n\
        width: calc(var(--heading-size) / sqrt(2)); \n\
        border-radius: 30%; \n\
        border: solid var(--sec-color) 1px;\n\
        align-content: center; \n\
        text-align: center; \n\
        font-weight: 600;\n\
        font-family: 'Google Sans Flex', sans-serif;\n\
        color: var(--sec-color);\n\
        rotate: 45deg;\n\
        overflow: hidden;\n\
    } \n\
    .list {\n\
        display: flex;\n\
        flex-direction: column;\n\
        gap: 10px;\n\
        margin-left: 20px;\n\
    }\n\
    .list div {\n\
        display: inline-block;\n\
        margin-left: 1em;\n\
    }\n\
    .list div:before {\n\
        position: absolute;\n\
        content: \"\";\n\
        display: block;\n\
        width: .5rem;\n\
        margin-left: -.8rem;\n\
        aspect-ratio: 1;\n\
        align-self: center;\n\
        rotate: 45deg;\n\
        border-radius: 30%;\n\
        background-color: var(--sec-color);\n\
    }\n\
    .code {\n\
        border-radius: 10px;\n\
        background: #121212;\n\
        padding: 20px;\n\
        border: solid 1px var(--sec-color);\n\
        font-family: 'JetBrains Mono';\n\
        font-size: .8em;\n\
        overflow: hidden;\n\
        margin-inline: calc(var(--heading-size) / 2);\n\
    }\n\
    .code:before {\n\
        content: attr(lang);\n\
        display: block;\n\
        background: #1b1b1b;\n\
        margin-top: -20px;\n\
        margin-inline: -20px;\n\
        padding-inline: 10px;\n\
        margin-bottom: 10px;\n\
    }\n\
    .code .line {\n\
        display: flex;\n\
        gap: 1ch;\n\
        padding-left: calc(var(--tabs) * 4ch);\n\
    }\n\
    .code .type {\n\
        color: #e5c890;\n\
    }\n\
    .code .keyword {\n\
        color: #ca9ee6;\n\
        font-style: italic;\n\
    }\n\
    </style>";

    for (const node* n : tree) {
        result += node_to_str(n);
    }

    result += "</html>";

    fputs(result.c_str(), out);

    fclose(out);
}
