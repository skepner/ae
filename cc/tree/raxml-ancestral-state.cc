#include <unordered_map>

#include "utils/file.hh"
#include "utils/string.hh"
#include "sequences/sequence.hh"
#include "sequences/translate.hh"
#include "tree/tree.hh"

// ======================================================================

class RaxmlAncestralState
{
  public:
    RaxmlAncestralState(const std::filesystem::path& filename) : data_{ae::file::read(filename)}
        {
            using namespace ae::sequences;
            for (const auto line : ae::string::split_into_lines(data_, ae::string::split_emtpy::strip_remove)) {
                const auto name_seq = ae::string::split(line, "\t");
                name_to_seq_.emplace(name_seq[0], sequence_pair_t{sequence_aa_t{translate_nucleotides_to_amino_acids(name_seq[1])}, sequence_nuc_t{name_seq[1]}});
            }
        }

        const ae::sequences::sequence_pair_t& get(std::string_view name) const
        {
            if (const auto found = name_to_seq_.find(name); found != name_to_seq_.end())
                return found->second;
            else
                return empty_;
        }

  private:
    const std::string data_;
    const ae::sequences::sequence_pair_t empty_{};
    std::unordered_map<std::string_view, ae::sequences::sequence_pair_t> name_to_seq_;

};

// return {diff_nuc, diff_aa}
inline std::pair<std::vector<size_t>, std::vector<size_t>> diff(const std::vector<const ae::sequences::sequence_pair_t*>& seqs)
{
    std::vector<size_t> diff_nuc, diff_aa;
    if (!seqs.empty()) {
        for (size_t pos{0}; pos < seqs[0]->nuc.size(); ++pos) {
            for (auto seq = std::next(seqs.begin()); seq != seqs.end(); ++seq) {
                if ((*seq)->nuc[pos] != seqs[0]->nuc[pos]) {
                    diff_nuc.push_back(pos);
                    break;
                }
            }
        }
        for (size_t pos{0}; pos < seqs[0]->aa.size(); ++pos) {
            for (auto seq = std::next(seqs.begin()); seq != seqs.end(); ++seq) {
                if ((*seq)->aa[pos] != seqs[0]->aa[pos]) {
                    diff_aa.push_back(pos);
                    break;
                }
            }
        }
    }
    return {diff_nuc, diff_aa};
}

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
                const auto& name = raxml_tree->leaf(node_index).name;
                if (const auto found = leaf_name_to_its_parent.find(name); found != leaf_name_to_its_parent.end())
                    found->second->raxml_inode_names.insert(parent->name);
            }
        }
    }

    RaxmlAncestralState state{raxml_states_file};

    for (auto inode_ref : visit(tree_visiting::inodes)) {
        auto* inode = inode_ref.inode();
        std::vector<const ae::sequences::sequence_pair_t*> sequences(inode->raxml_inode_names.size());
        std::transform(inode->raxml_inode_names.begin(), inode->raxml_inode_names.end(), sequences.begin(), [&state](std::string_view name) { return &state.get(name); });
        const auto cmp = [](const ae::sequences::sequence_pair_t* e1, const ae::sequences::sequence_pair_t* e2) { return e1->nuc < e2->nuc; };
        const auto eq = [](const ae::sequences::sequence_pair_t* e1, const ae::sequences::sequence_pair_t* e2) { return e1->nuc == e2->nuc; };
        std::sort(sequences.begin(), sequences.end(), cmp);
        sequences.erase(std::unique(sequences.begin(), sequences.end(), eq), sequences.end());
        if (sequences.size() > 1) {
            const auto [diff_nuc, diff_aa] = diff(sequences);
            AD_WARNING("{} diff nuc {} aa {} inode {}", inode->node_id_, diff_nuc, diff_aa, inode->raxml_inode_names);
        }
    }

} // ae::tree::Tree::set_raxml_ancestral_state_reconstruction_data

// ----------------------------------------------------------------------
