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
    struct seqpair_t
    {
        ae::sequences::sequence_aa_t aa;
        ae::sequences::sequence_nuc_t nuc;
    };

    RaxmlAncestralState(const std::filesystem::path& filename) : data_{ae::file::read(filename)}
    {
        using namespace ae::sequences;
        for (const auto line : ae::string::split_into_lines(data_, ae::string::split_emtpy::strip_remove)) {
            const auto name_seq = ae::string::split(line, "\t");
            name_to_seq_.emplace(name_seq[0], seqpair_t{sequence_aa_t{translate_nucleotides_to_amino_acids(name_seq[1])}, sequence_nuc_t{name_seq[1]}});
        }
    }

    const seqpair_t& get(std::string_view name) const
    {
        if (const auto found = name_to_seq_.find(name); found != name_to_seq_.end())
            return found->second;
        else
            return empty_;
    }

  private:
    const std::string data_;
    const seqpair_t empty_{};
    std::unordered_map<std::string_view, seqpair_t> name_to_seq_;
};

inline auto operator<=>(const RaxmlAncestralState::seqpair_t& s1, const RaxmlAncestralState::seqpair_t& s2)
{
    return s1.nuc <=> s2.nuc;
}

inline auto operator==(const RaxmlAncestralState::seqpair_t& s1, const RaxmlAncestralState::seqpair_t& s2)
{
    return s1.nuc == s2.nuc;
}

// return {diff_nuc, diff_aa}
inline std::pair<std::vector<size_t>, std::vector<size_t>> diff(const std::vector<std::reference_wrapper<const RaxmlAncestralState::seqpair_t>>& seqs)
{
    std::vector<size_t> diff_nuc, diff_aa;
    if (!seqs.empty()) {
        for (size_t pos{0}; pos < seqs[0].get().nuc.size(); ++pos) {
            for (auto seq = std::next(seqs.begin()); seq != seqs.end(); ++seq) {
                if (seq->get().nuc[pos] != seqs[0].get().nuc[pos]) {
                    diff_nuc.push_back(pos);
                    break;
                }
            }
        }
        for (size_t pos{0}; pos < seqs[0].get().aa.size(); ++pos) {
            for (auto seq = std::next(seqs.begin()); seq != seqs.end(); ++seq) {
                if (seq->get().aa[pos] != seqs[0].get().aa[pos]) {
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

    // populate inodes with sequences
    for (auto inode_ref : visit(tree_visiting::inodes)) {
        auto* inode = inode_ref.inode();
        std::vector<std::reference_wrapper<const RaxmlAncestralState::seqpair_t>> sequences;
        std::transform(inode->raxml_inode_names.begin(), inode->raxml_inode_names.end(), std::back_inserter(sequences),
                       [&state](std::string_view name) -> const RaxmlAncestralState::seqpair_t& { return std::cref(state.get(name)); });
        if (sequences.size() > 1) {
            std::sort(sequences.begin(), sequences.end());
            sequences.erase(std::unique(sequences.begin(), sequences.end()), sequences.end());
        }
        switch (sequences.size()) {
            case 0:
                // AD_WARNING("no inode {} sequences found", inode->node_id_);
                break;
            case 1:
                inode->aa = sequences[0].get().aa;
                inode->nuc = sequences[0].get().nuc;
                break;
            default:
                inode->aa = sequences[0].get().aa;
                inode->nuc = sequences[0].get().nuc;
                {
                    const auto [diff_nuc, diff_aa] = diff(sequences);
                    AD_WARNING("{} diff nuc {} aa {} inode {}", inode->node_id_, diff_nuc, diff_aa, inode->raxml_inode_names);
                }
                break;
        }
    }

    set_transition_labels_by_raxml_ancestral_state_reconstruction_data();

} // ae::tree::Tree::set_raxml_ancestral_state_reconstruction_data

// ----------------------------------------------------------------------

void ae::tree::Tree::set_transition_labels_by_raxml_ancestral_state_reconstruction_data()
{

} // ae::tree::Tree::set_transition_labels_by_raxml_ancestral_state_reconstruction_data

// ----------------------------------------------------------------------
