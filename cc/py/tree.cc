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
        ;

    tree_submodule.def("load", [](pybind11::object path) { return load(std::string{pybind11::str(path)}); }, "filename"_a);
    tree_submodule.def("export", [](const Tree& tree, pybind11::object path) { export_tree(tree, std::string{pybind11::str(path)}); }, "tree"_a, "filename"_a);

    // ----------------------------------------------------------------------
}

// ======================================================================
