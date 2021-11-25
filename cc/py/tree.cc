#include "tree/tree.hh"
#include "py/module.hh"

// ======================================================================

namespace ae::tree
{
    struct Node_Ref
    {
        node_index_t node_index;
        Tree& tree;

        std::string name() const
        {
            return tree.node(node_index).visit([](const auto* node) { return node->name; });
        }
        double edge() const
        {
            return tree.node(node_index).visit([](const auto* node) -> double { return node->edge.get(); });
        }
        double cumulative_edge() const
        {
            return tree.node(node_index).visit([](const auto* node) -> double { return node->cumulative_edge.get(); });
        }
        node_index_base_t node_id() const
        {
            return tree.node(node_index).visit([](const auto* node) -> node_index_base_t { return node->node_id_.get(); });
        }
    };

    struct Nodes_Iterator
    {
        const Nodes& nodes;
        size_t index{0};

        Node_Ref next()
        {
            if (index == nodes.nodes.size())
                throw pybind11::stop_iteration();
            return {nodes.nodes[index++], nodes.tree};
        }
    };

} // namespace ae::tree

// ======================================================================

void ae::py::tree(pybind11::module_& mdl)
{
    using namespace std::string_view_literals;
    using namespace pybind11::literals;
    using namespace ae::tree;

    // ----------------------------------------------------------------------

    auto tree_submodule = mdl.def_submodule("tree", "tree manipulation");

    pybind11::class_<Tree, std::shared_ptr<Tree>>(tree_submodule, "Tree") //
        .def(
            "populate_with_sequences", [](Tree& tree, std::string_view subtype) { tree.populate_with_sequences(virus::type_subtype_t{subtype}); }, "subtype"_a) //
        .def(
            "populate_with_duplicates", [](Tree& tree, std::string_view subtype) { tree.populate_with_duplicates(virus::type_subtype_t{subtype}); }, "subtype"_a) //
        .def("select_all", &Tree::select_all)                                                                                                                     //
        .def("select_leaves", &Tree::select_leaves)                                                                                                               //
        .def("select_inodes", &Tree::select_inodes)                                                                                                               //
        .def(
            "remove",
            [](Tree& tree, std::vector<Node_Ref>& nodes) {
                std::vector<node_index_t> indexes(nodes.size());
                std::transform(std::begin(nodes), std::end(nodes), std::begin(indexes), [&tree](const auto& ref) {
                    if (&ref.tree != &tree)
                        throw std::invalid_argument{"nodes are not from the passed tree"};
                    return ref.node_index;
                });
                tree.remove(indexes);
            },
            "nodes"_a) //
        ;

    pybind11::class_<Nodes>(tree_submodule, "Nodes")                                                       //
        .def("sort_by_cumulative", &Nodes::sort_by_cumulative)                                             //
        .def("filter_by_cumulative_more_than", &Nodes::filter_by_cumulative_more_than, "min_cumulative"_a) //
        .def("remove", &Nodes::remove)                                                                     //
        .def("__len__", [](const Nodes& nodes) { return nodes.nodes.size(); })                             //
        .def(
            "__iter__", [](const Nodes& nodes) { return Nodes_Iterator{nodes}; }, pybind11::keep_alive<0, 1>()) //
        .def("__getitem__",
             [](const Nodes& nodes, size_t index) {
                 if (index >= nodes.nodes.size())
                     throw std::out_of_range(fmt::format("index {} is out of range for Nodes, max index allowed: {}", index, nodes.nodes.size() - 1));
                 return Node_Ref{nodes.nodes[index], nodes.tree};
             }) //
        ;

    pybind11::class_<Nodes_Iterator>(tree_submodule, "Nodes_Iterator")             //
        .def("__iter__", [](Nodes_Iterator& it) -> Nodes_Iterator& { return it; }) //
        .def("__next__", &Nodes_Iterator::next)                                    //
        ;

    pybind11::class_<Node_Ref>(tree_submodule, "Node_Ref")  //
        .def("name", &Node_Ref::name)                       //
        .def("edge", &Node_Ref::edge)                       //
        .def("cumulative_edge", &Node_Ref::cumulative_edge) //
        .def("node_id", &Node_Ref::node_id)                 //
        ;

    tree_submodule.def(
        "load", [](pybind11::object path) { return load(std::string{pybind11::str(path)}); }, "filename"_a);
    tree_submodule.def(
        "export", [](const Tree& tree, pybind11::object path) { export_tree(tree, std::string{pybind11::str(path)}); }, "tree"_a, "filename"_a);

    // ----------------------------------------------------------------------
}

// ======================================================================
