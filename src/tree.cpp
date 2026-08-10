#include "tree.hpp"
#include "log.hpp"
#include "node.hpp"
#include "uninitialized_tree.hpp"
#include "value.hpp"
#include <string>

// Pastes child nodes in every place where utree[i].is_insert()
void paste_children(std::vector<modoc::uninitialized_tree::unode>& utree, const std::vector<modoc::uninitialized_tree::unode>& children) {
    for (auto itr = utree.begin(); itr != utree.end(); ++itr) {
        modoc::uninitialized_tree::unode& un = *itr;

        if (un.is_node() && un.node().children.size()) paste_children(un.node().children, children);
        else if (un.is_insert()) {
            itr = utree.erase(itr);
            itr = utree.insert(itr, children.begin(), children.end());
            itr += children.size() - 1;
        }
    } 
}

std::vector<node*> modoc::tree::initialize_node(tree& tree, uninitialized_tree::unode& un, const uint8_t depth, const bool copy_text) {
    if (un.is_node()) {
        uninitialized_tree::unode::node_type& nt = un.node();
        const std::function<const value*(std::string_view)> get_var_func = [&tree](std::string_view name) {return tree.get_variable(name);};
        options_t map;

        {
            std::string_view options = nt.options.view();
            if (options.size()) {
                parse_options(options, map, get_var_func);
                //for (auto& e : map)
                //    printf("[%.*s : %s]\n", (int)e.first.size(), e.first.data(), e.second.to_string().c_str());
            }
        }

        //printf("[%.*s]\n", (int)nt.node_name.size(), nt.node_name.data());
        node* n = node_factories.at(nt.node_name.view())->instance(depth, map);
       
        {
            std::string_view meta = nt.meta.view();
            if (meta.size()) {
                map.clear();
                parse_options(meta, map, get_var_func);
                //for (auto& e : map)
                //    printf("{%.*s : %s}\n", (int)e.first.size(), e.first.data(), e.second.to_string().c_str());
                n->add_meta(map);
            }
        }

        // Handling the first child text node
        if (n->scope_end() != scope_end::START && nt.children.size() && nt.children.front().is_text()) {
            modoc::string_type& str = nt.children.front().text();
            const std::string_view view = str.view();
            const char* end;

            switch (n->scope_end()) {
                case scope_end::ENDL:
                    end = view.data();
                    while (*end != '\n' && end != view.end()) ++end; // Move to endl or end of text
                    break;
                case scope_end::ENDSCP:
                    end = view.end(); 
            }

            if (n->verbatim()) n->parse_verbatim({view.begin(), end});
            else n->parse_tokens(tokenize({view.begin(), end}, get_var_func), 0);

            while (end < view.end() && (*end == ' ' || *end == '\n' || *end == '\t')) ++end; // Skip invalid chars

            if (end < view.end()) {
                str.remove_prefix(end - view.begin()); // Move begin to end
                //printf("remaining: [%.*s]\n", (int)str.view().size(), str.view().data());
            }
            else {
                nt.children.erase(nt.children.begin()); // Remove text node
                //puts("<erased>");
            }
        }

        if (n->is_primitive()) {
            modoc::tree initialized = std::move(initialize(nt.children, depth + 1, copy_text));
            
            for (node* child : initialized.nodes) n->add_node(child);
            initialized.nodes.clear();

            //printf("Initialized primitive [%s]\n", n->type());
            //fflush(stdout);

            /*tree.nodes.push_back(n);
            for (auto entry : tree.variables) {
                value v = entry.second.top().v;
                tree.assign(entry.first, std::move(v));
            }*/


            return {n};
        }
        else {
            std::vector<uninitialized_tree::unode> expanded = ((special_node*)n)->expand(tree);
            //printf("Initialized special [%s]\n", n->type());
            //fflush(stdout);
            delete n;

            paste_children(expanded, nt.children);
            
            modoc::tree initialized = std::move(modoc::tree::initialize(expanded, depth, true));
            std::vector<node*> result = std::move(initialized.nodes);
            initialized.nodes.clear();

            const uint32_t off = tree.nodes.size();
            for (auto& entry : initialized.variables) {
                auto stack = entry.second;

                while (stack.size()) {
                    tree._assign_at(entry.first, std::move(stack.top().v), off + stack.top().begin_ind);
                    stack.pop();
                }
            }

            return result;
        }
    }
    else return {new text_node(tokenize(un.text().view(), nullptr, copy_text))}; // Maybe add a check if tokenize returns an empty vector. For instance a variable could evaluate to an empty string.
}

modoc::tree modoc::tree::initialize(std::vector<uninitialized_tree::unode>& unodes, const uint8_t depth, const bool copy_text) {
    tree result;

    for (uninitialized_tree::unode& un : unodes) {
        std::vector<node*> initialized = initialize_node(result, un, depth, copy_text);
        std::move(initialized.begin(), initialized.end(), std::back_inserter(result.nodes));
        initialized.clear();
    }

    return result;
}

/*void modoc::tree::print_node(const node* n, std::list<bool>& branch_end, bool is_list_elm, size_t nest) const {
        for (std::list<bool>::iterator itr = branch_end.begin(); itr != --branch_end.end(); ++itr) {
            if (!*itr) fputs("\u2502  ", stdout);
            else fputs("   ", stdout);
        }

        if (!branch_end.back()) fputs("\u251C", stdout);
        else fputs("\u2514", stdout); 

        if (is_list_elm) fputs("\u2500\u25A1", stdout); // \u25CF - full circle  \u25EF - circle
        else fputs("\u2500\u2500", stdout);

        printf("[%u]", n->type_id());
        n->debug_print();
        ++nest;

        const std::vector<node*>* children = n->child_nodes();
        
        if (children != nullptr) {
            branch_end.push_back(false);
            for (size_t i = 0; i < children->size(); ++i) {
                branch_end.back() = (i == children->size() - 1);
                print_node((*children)[i], branch_end, debug_str_match(n->type(), "list"), nest);
            }
            branch_end.pop_back();
        }
    }*/

std::string modoc::tree::node_to_str(const node* n, std::list<bool>& branch_end, bool is_list_elm, size_t nest) const {
    std::string result = "\033[2m";

    for (std::list<bool>::iterator itr = branch_end.begin(); itr != --branch_end.end(); ++itr) {
        if (!*itr) result += "\u2502  ";
        else result += "   ";
    }

    if (!branch_end.back()) result += "\u251C";
    else result += "\u2514"; 

    if (is_list_elm) result += "\u2500\u25A1"; // \u25CF - full circle  \u25EF - circle
    else result += "\u2500\u2500";

    result += "\033[0m";

    /*result += '[';
    result += std::to_string(n->type_id());
    result += ']';*/
    result += n->to_string();
    result += '\n';
    ++nest;

    const std::vector<node*>* children = n->child_nodes();
    
    if (children != nullptr) {
        branch_end.push_back(false);
        for (size_t i = 0; i < children->size(); ++i) {
            branch_end.back() = (i == children->size() - 1);
            result += node_to_str((*children)[i], branch_end, debug_str_match(n->type(), "list"), nest);
        }
        branch_end.pop_back();
    }

    return result;
}


void modoc::tree::destroy_node(node* n) {
    const std::vector<node*>* children = n->child_nodes();

    if (children != nullptr)
        for (node* ch : *children)
            destroy_node(ch);

    delete n;
}
