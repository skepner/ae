#include "tree/tree.hh"

// ======================================================================

// begin
template <ae::lvalue_reference TREE, ae::pointer LEAF, ae::pointer INODE>
ae::tree::tree_iterator_t<TREE, LEAF, INODE>::tree_iterator_t(TREE tree, tree_visiting a_visiting) : tree_{tree}, visiting_{a_visiting}
{
    parents_.reserve(tree.depth());
    parents_.emplace_back(node_index_t{0}, parent_itself);
    switch (visiting_) {
        case tree_visiting::all:
        case tree_visiting::inodes:
            break;
        case tree_visiting::leaves:
        case tree_visiting::all_post:
        case tree_visiting::inodes_post:
            operator++();
            break;
    }
}

// ----------------------------------------------------------------------

// end
template <ae::lvalue_reference TREE, ae::pointer LEAF, ae::pointer INODE>
ae::tree::tree_iterator_t<TREE, LEAF, INODE>::tree_iterator_t(TREE tree, tree_visiting a_visiting, _init_end) : tree_{tree}, visiting_{a_visiting}
{
    parents_.emplace_back(node_index_t{0}, iteration_end);
}

// ----------------------------------------------------------------------

template <ae::lvalue_reference TREE, ae::pointer LEAF, ae::pointer INODE> ae::tree::tree_iterator_t<TREE, LEAF, INODE>& ae::tree::tree_iterator_t<TREE, LEAF, INODE>::operator++()
{
    // returns if suitable node found after diving
    const auto dive = [this](size_t child_no) -> bool {
        auto* parent = &parents_.back();
        parent->second = child_no;
        if (child_no != parent_itself) {
            if (const auto& parent_inode = tree_.inode(parent->first); child_no >= parent_inode.children.size())
                return is_visiting_post();
            else if (const auto child_index = parent_inode.children[child_no]; !is_leaf(child_index)) {
                parent = &parents_.emplace_back(child_index, parent_itself);
                child_no = parent_itself;
            }
        }
        switch (visiting_) {
            case tree_visiting::all:
                return true;
            case tree_visiting::inodes:
                return child_no == parent_itself;
            case tree_visiting::leaves:
            case tree_visiting::all_post:
                return child_no != parent_itself;
            case tree_visiting::inodes_post:
                return false;
        }
    };

    // ----------------------------------------------------------------------

    while (true) {
        auto parent = parents_.end() - 1;
        if (parent->second == parent_itself) {
            if (dive(0))
                break;
        }
        else if (const auto& parent_inode = tree_.inode(parent->first); parent->second < parent_inode.children.size()) {
            ++parent->second;
            if (parent->second < parent_inode.children.size()) {
                if (dive(parent->second))
                    break;
            }
            else if (is_visiting_post()) {
                break;
            }
        }
        else if (parents_.size() == 1) { // *post -> end
            parent->second = iteration_end;
            break;
        }
        else { // undive
            parents_.pop_back();
            parent = parents_.end() - 1;
            if (parents_.size() == 1) {
                if (is_visiting_post())
                    ++parent->second; // *post -> pre-end
                else
                    parent->second = iteration_end; // -> end
                break;
            }
            else if (dive(parent->second + 1))
                break;
        }
    }
    return *this;
}

// ----------------------------------------------------------------------

template <ae::lvalue_reference TREE, ae::pointer LEAF, ae::pointer INODE> typename ae::tree::tree_iterator_t<TREE, LEAF, INODE>::reference ae::tree::tree_iterator_t<TREE, LEAF, INODE>::operator*()
{
    auto& [parent_index, child_no] = parents_.back();
    auto& parent = tree_.inode(parent_index);
    if (child_no == parent_itself)
        return &parent;
    else if (child_no < parent.children.size())
        return tree_.node(parent.children[child_no]);
    else if (is_visiting_post() && child_no != iteration_end)
        return &parent;
    else
        throw std::runtime_error{"tree_iterator_t: derefencing at the end?"};
}

// ======================================================================

template class ae::tree::tree_iterator_t<ae::tree::Tree&, ae::tree::Leaf*, ae::tree::Inode*>;
template class ae::tree::tree_iterator_t<const ae::tree::Tree&, const ae::tree::Leaf*, const ae::tree::Inode*>;

// ======================================================================
