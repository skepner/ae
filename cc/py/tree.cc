#include "tree/tree.hh"
#include "py/module.hh"

// ======================================================================

namespace ae::tree
{
}

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
        .def("leaves_by_cumulative", [](Tree& tree) { return tree.leaves_by_cumulative(); })                                                                             //
        ;

    pybind11::class_<Nodes>(tree_submodule, "Nodes")                           //
        .def("__len__", [](const Nodes& nodes) { return nodes.nodes.size(); }) //
        // .def("__getitem__", [](const Nodes& nodes, size_t index) { return nodes.nodes.size(); }) //
        .def(
            "name", [](const Nodes& nodes, size_t index) { return nodes.tree.node(nodes.nodes[index]).visit([](auto* node) { return node->name; }); }, "index"_a) //
        .def(
            "edge", [](const Nodes& nodes, size_t index) { return nodes.tree.node(nodes.nodes[index]).visit([](auto* node) { return node->edge.get(); }); }, "index"_a) //
        .def(
            "cumulative_edge", [](const Nodes& nodes, size_t index) { return nodes.tree.node(nodes.nodes[index]).visit([](auto* node) { return node->cumulative_edge.get(); }); }, "index"_a) //
        .def(
            "node_id", [](const Nodes& nodes, size_t index) { return nodes.tree.node(nodes.nodes[index]).visit([](auto* node) { return node->node_id_.get(); }); }, "index"_a) //
        ;

    tree_submodule.def(
        "load", [](pybind11::object path) { return load(std::string{pybind11::str(path)}); }, "filename"_a);
    tree_submodule.def(
        "export", [](const Tree& tree, pybind11::object path) { export_tree(tree, std::string{pybind11::str(path)}); }, "tree"_a, "filename"_a);

    // ----------------------------------------------------------------------
}

// ======================================================================
