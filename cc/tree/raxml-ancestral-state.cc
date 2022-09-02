#include <algorithm>
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
    std::unordered_map<std::string_view, seqpair_t> name_to_seq_{};
};

inline auto operator<=>(const RaxmlAncestralState::seqpair_t& s1, const RaxmlAncestralState::seqpair_t& s2)
{
    return s1.nuc <=> s2.nuc;
}

inline auto operator==(const RaxmlAncestralState::seqpair_t& s1, const RaxmlAncestralState::seqpair_t& s2)
{
    return s1.nuc == s2.nuc;
}

inline std::vector<size_t> diff(const std::vector<std::string_view>& seqs)
{
    std::vector<size_t> diff;
    if (!seqs.empty()) {
        for (size_t pos{0}; pos < seqs[0].size(); ++pos) {
            for (auto seq = std::next(seqs.begin()); seq != seqs.end(); ++seq) {
                if ((*seq)[pos] != seqs[0][pos]) {
                    diff.push_back(pos);
                    break;
                }
            }
        }
    }
    return diff;
}

template <typename Seq> std::vector<std::string_view> sequences(const std::vector<std::reference_wrapper<const RaxmlAncestralState::seqpair_t>>& source)
{
    std::vector<std::string_view> seqs;
    for (const auto& src : source) {
        if constexpr (std::is_same_v<Seq, ae::sequences::sequence_aa_t>)
            seqs.emplace_back(src.get().aa);
        else
            seqs.emplace_back(src.get().nuc);
    }
    return seqs;
}

// return {diff_nuc, diff_aa}
inline std::pair<std::vector<size_t>, std::vector<size_t>> diff(const std::vector<std::reference_wrapper<const RaxmlAncestralState::seqpair_t>>& seqs)
{
    return {diff(sequences<ae::sequences::sequence_nuc_t>(seqs)), diff(sequences<ae::sequences::sequence_aa_t>(seqs))};
}

// ======================================================================

void ae::tree::Tree::set_raxml_ancestral_state_reconstruction_data(const std::filesystem::path& raxml_tree_file, const std::filesystem::path& raxml_states_file)
{
    // AD_DEBUG("\"{}\" \"{}\"", raxml_tree_file, raxml_states_file);

    std::unordered_map<std::string_view, Inode*> leaf_name_to_its_parent;
    for (auto inode_ref : visit(tree_visiting::inodes)) {
        auto* parent = inode_ref.inode();
        for (const auto child_index : parent->children) {
            if (is_leaf(child_index))
                leaf_name_to_its_parent.emplace(std::string_view{leaf(child_index).name}, parent);
        }
    }

    auto raxml_tree = load(raxml_tree_file);

    // populate raxml_inode_names in this tree
    for (auto inode_ref : raxml_tree->visit(tree_visiting::inodes)) {
        auto* parent = inode_ref.inode();
        for (const auto child_index : parent->children) {
            if (is_leaf(child_index)) {
                const auto& name = raxml_tree->leaf(child_index).name;
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
                // no inode sequences
                // see set_inode_sequences_if_no_ancestral_data()
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

    set_inode_sequences_if_no_ancestral_data();
    set_transition_labels_by_raxml_ancestral_state_reconstruction_data();
    check_transition_label_flip();

} // ae::tree::Tree::set_raxml_ancestral_state_reconstruction_data

// ----------------------------------------------------------------------

template <typename Seq> std::vector<std::string_view> child_sequences(const ae::tree::Tree& tree, const ae::tree::Inode& parent)
{
    std::vector<std::string_view> seqs;
    for (const auto child_index : parent.children) {
        const auto& child = tree.node(child_index);
        if (const auto& seq = child.node()->get_aa_nuc<Seq>(); !seq.empty())
            seqs.push_back(seq);
        else if (!ae::tree::is_leaf(child_index)) {
            const auto grandchildren = child_sequences<Seq>(tree, tree.inode(child_index));
            seqs.insert(seqs.end(), grandchildren.begin(), grandchildren.end());
        }
    }
    return seqs;
}

inline std::string construct_missing_sequence(std::string_view parent, const std::vector<std::string_view>& children)
{
    if (children.empty())
        return std::string{parent};

    // generate sequence by comparing child nodes sequences, for positions where child node sequences are different use parent node aa/nuc

    std::string result{children[0]};
    AD_WARNING(children.size() < 2, "[construct_missing_sequence] too few children: {} diff {}", children.size(), diff(children));
    for (size_t pos : diff(children))
        result[pos] = parent[pos];
    return result;
}

void ae::tree::Tree::set_inode_sequences_if_no_ancestral_data()
{
    // looks like raxml does not put sequence into a inode when node has just inode children
    // generate sequence by comparing child nodes sequences, for positions where child node sequences are different use parent node aa/nuc

    for (auto inode_ref : visit(tree_visiting::inodes)) {
        if (auto* inode = inode_ref.inode(); inode->aa.empty()) {
            // AD_WARNING("no inode {} sequences found", inode->node_id_);
            if (const auto& parent_inode = this->inode(parent(inode_ref.node_id())); !parent_inode.aa.empty()) {
                inode->aa = ae::sequences::sequence_aa_t{construct_missing_sequence(parent_inode.aa, child_sequences<ae::sequences::sequence_aa_t>(*this, *inode))};
                inode->nuc = ae::sequences::sequence_nuc_t{construct_missing_sequence(parent_inode.nuc, child_sequences<ae::sequences::sequence_nuc_t>(*this, *inode))};
                // AD_DEBUG("inode {} seq set [{}]", inode->node_id_, inode->aa);
            }
            else
                AD_WARNING("neither inode {} nor its parent {} sequences found", inode->node_id_, parent_inode.node_id_);
        }
    }

} // ae::tree::Tree::set_inode_sequences_if_no_ancestral_data

// ----------------------------------------------------------------------

void ae::tree::Tree::set_transition_labels_by_raxml_ancestral_state_reconstruction_data()
{
    for (auto inode_ref : visit(tree_visiting::inodes)) {
        const auto* parent = inode_ref.inode();
        AD_WARNING(parent->aa.empty(), "[set_transition_labels_by_raxml_ancestral_state_reconstruction_data] parent {} has empty sequence", parent->node_id_);
        for (const auto child_index : parent->children) {
            if (!is_leaf(child_index)) {
                auto& child = inode(child_index);
                child.aa_transitions.clear();
                child.nuc_transitions.clear();
                for (const auto [pos1, aa] : ae::sequences::indexed(child.aa)) {
                    if (aa != parent->aa[pos1])
                        child.aa_transitions.add(parent->aa[pos1], pos1, aa);
                }
                // AD_DEBUG(!child.aa_transitions.empty(), "{} {}", child.node_id_, child.aa_transitions);
                for (const auto [pos1, nuc] : ae::sequences::indexed(child.nuc)) {
                    if (nuc != parent->nuc[pos1])
                        child.nuc_transitions.add(parent->nuc[pos1], pos1, nuc);
                }
            }
        }
    }

} // ae::tree::Tree::set_transition_labels_by_raxml_ancestral_state_reconstruction_data

// ----------------------------------------------------------------------

void ae::tree::Tree::check_transition_label_flip()
{
    const auto extract_pos = [](const transitions_t& transitions) {
        std::vector<sequences::pos1_t> pos;
        for (const auto& tr : transitions.transitions)
            pos.push_back(tr.pos);
        return pos;
    };

    const auto check = [extract_pos](const Inode& parent, const Inode& child) {
        const auto parent_pos = extract_pos(parent.aa_transitions), child_pos = extract_pos(child.aa_transitions);
        std::vector<sequences::pos1_t> common;
        std::set_intersection(parent_pos.begin(), parent_pos.end(), child_pos.begin(), child_pos.end(), std::back_inserter(common));
        if (!common.empty())
            AD_WARNING("potential aa transition label flip: parent: {} {}   child: {} {}", parent.node_id_, parent.aa_transitions, child.node_id_, child.aa_transitions);
    };

    for (auto inode_ref : visit(tree_visiting::inodes)) {
        if (const auto* parent = inode_ref.inode(); !parent->aa_transitions.empty()) {
            for (const auto child_index : parent->children) {
                if (!is_leaf(child_index)) {
                    if (auto& child = inode(child_index); !child.aa_transitions.empty())
                        check(*parent, child);
                }
            }
        }
    }

} // ae::tree::Tree::check_transition_label_flip

// ----------------------------------------------------------------------
