#include "tree.hpp"
#include "node.hpp"
#include "uninitialized_tree.hpp"
#include "value.hpp"

// Pastes child nodes in every place where utree[i].is_insert()
void paste_children(std::vector<modoc::uninitialized_tree::unode>& utree, std::vector<modoc::uninitialized_tree::unode> children) {
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

std::vector<node*> modoc::tree::initialize_node(tree& tree, const uninitialized_tree::unode& un, const uint8_t depth) {
    if (un.is_node()) {
        const uninitialized_tree::unode::node_type& nt = un.node();
        options_t map;

        if (nt.options.size()) {
            parse_options(nt.options, map);
            for (auto& e : map)
                printf("[%.*s : %s]\n", (int)e.first.size(), e.first.data(), e.second.to_string().c_str());
        }

        printf("[%.*s]\n", (int)nt.node_name.size(), nt.node_name.data());
        node* n = node_factories.at(nt.node_name)->instance(depth, map);
        
        if (nt.meta.size()) {
            map.clear();
            parse_options(nt.meta, map);
            for (auto& e : map)
                printf("{%.*s : %s}\n", (int)e.first.size(), e.first.data(), e.second.to_string().c_str());
            n->add_meta(map);
        }

        if (n->is_primitive()) {
            modoc::tree initialized = initialize(nt.children, depth + 1);
            
            for (node* child : initialized.nodes) n->add_node(child);
            initialized.nodes.clear();

            return {n};
        }
        else {
            std::vector<uninitialized_tree::unode> expanded = ((special_node*)n)->expand(tree);
            delete n;

            paste_children(expanded, nt.children);
            
            modoc::tree initialized = modoc::tree::initialize(expanded, depth);
            std::vector<node*> result = std::move(initialized.nodes);
            initialized.nodes.clear();

            return result;
        }
    }
    else return {new text_node(tokenize(un.text()))};
}

modoc::tree modoc::tree::initialize(const std::vector<uninitialized_tree::unode>& unodes, const uint8_t depth) {
    tree result;

    for (const uninitialized_tree::unode& un : unodes) {
        std::vector<node*> initialized = initialize_node(result, un, depth);
        std::move(initialized.begin(), initialized.end(), std::back_inserter(result.nodes));
    }

    return result;
}

void modoc::tree::print_node(const node* n, std::list<bool>& branch_end, bool is_list_elm, size_t nest) const {
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
    }

void modoc::tree::destroy_node(node* n) {
    const std::vector<node*>* children = n->child_nodes();

    if (children != nullptr)
        for (node* ch : *children)
            destroy_node(ch);

    delete n;
}
