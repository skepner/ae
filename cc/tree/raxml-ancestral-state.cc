#include <unordered_map>

#include "utils/file.hh"
#include "utils/string.hh"
#include "tree/tree.hh"

// ======================================================================

class RaxmlAncestralState
{
  public:
    RaxmlAncestralState(const std::filesystem::path& filename) : data_{ae::file::read(filename)}
        {
            for (const auto line : ae::string::split_into_lines(data_)) {
                const auto name_seq = ae::string::split(line, "\t");
                name_to_seq_.emplace(name_seq[0], name_seq[1]);
            }
        }

        std::string_view get(std::string_view name) const
        {
            if (const auto found = name_to_seq_.find(name); found != name_to_seq_.end())
                return found->second;
            else
                return {};
        }

  private:
    const std::string data_;
    std::unordered_map<std::string_view, std::string_view> name_to_seq_;

};

// ======================================================================

void ae::tree::Tree::set_raxml_ancestral_state_reconstruction_data(const std::filesystem::path& raxml_tree_file, const std::filesystem::path& raxml_states_file)
{
    // AD_DEBUG("\"{}\" \"{}\"", raxml_tree_file, raxml_states_file);

    std::unordered_map<std::string_view, Inode*> leaf_name_to_its_parent;
    for (auto inode_ref : visit(tree_visiting::inodes)) {
        auto* parent = inode_ref.inode();
        for (const auto node_index : parent->children) {
            if (is_leaf(node_index))
                leaf_name_to_its_parent.emplace(std::string_view{leaf(node_index).name}, parent);
        }
    }

    auto raxml_tree = load(raxml_tree_file);

    // populate raxml_inode_names in this tree
    for (auto inode_ref : raxml_tree->visit(tree_visiting::inodes)) {
        auto* parent = inode_ref.inode();
        for (const auto node_index : parent->children) {
            if (is_leaf(node_index)) {
                const auto& name = leaf(node_index).name;
                if (const auto found = leaf_name_to_its_parent.find(name); found != leaf_name_to_its_parent.end())
                    found->second->raxml_inode_names.insert(parent->name);
            }
        }
    }

    RaxmlAncestralState state{raxml_states_file};

    for (auto inode_ref : visit(tree_visiting::inodes)) {
        auto* inode = inode_ref.inode();
        std::vector<std::string_view> sequences(inode->raxml_inode_names.size());
        std::transform(inode->raxml_inode_names.begin(), inode->raxml_inode_names.end(), sequences.begin(), [&state](std::string_view name) { return state.get(name); });
        std::sort(sequences.begin(), sequences.end());
        sequences.erase(std::unique(sequences.begin(), sequences.end()), sequences.end());
        if (sequences.size() > 1) {
            const auto diffs = ae::string::diff(sequences);
            AD_WARNING("diffs {} inode {}", diffs, inode->raxml_inode_names);
        }
    }

} // ae::tree::Tree::set_raxml_ancestral_state_reconstruction_data

// ----------------------------------------------------------------------
