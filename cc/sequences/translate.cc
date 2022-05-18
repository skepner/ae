#include <unordered_map>

#include "ext/range-v3.hh"
#include "utils/string.hh"

#include "utils/messages.hh"
#include "utils/string-hash.hh"
#include "sequences/translate.hh"
#include "sequences/raw-sequence.hh"

// ----------------------------------------------------------------------

static std::string translate_nucleotides_to_amino_acids(std::string_view nucleotides, size_t offset);
namespace ae::sequences
{
    static void aa_trim_absent(RawSequence& sequence, Messages& messages);
}

// ----------------------------------------------------------------------

// Some sequences from CNIC (and perhaps from other labs) have initial
// part of nucleotides with stop codons inside. To figure out correct
// translation we have first to translate with all possible offsets
// (0, 1, 2) and not stoppoing at stop codons, then try to align all
// of them. Most probably just one offset leads to finding correct
// align shift.

bool ae::sequences::translate(RawSequence& sequence, Messages& messages)
{
    constexpr size_t MINIMUM_SEQUENCE_AA_LENGTH = 200; // throw away everything shorter, HA1 is kinda 318, need to have just HA1 sequences to be able to make HA1 trees

    if (!sequence.raw_sequence.empty()) {
        // raw_sequence may be aa

        struct translated_t
        {
            std::string longest{}; // translated longest part
            size_t offset{};       // nuc prefix size at which translation started
        };
        // using translated_t = std::tuple<std::string, size_t>; // translated longest part, nuc prefix size at which translation started

        const auto find_longest_part = [](std::string&& aa, size_t offset) -> translated_t {
            auto split_data = ae::string::split(aa, "*");
            const auto longest_part = std::max_element(std::begin(split_data), std::end(split_data), [](const auto& e1, const auto& e2) { return e1.size() < e2.size(); });
            return {std::string(*longest_part), offset + static_cast<size_t>(longest_part->data() - aa.data()) * 3};
        };

        const auto transformation = [&sequence, find_longest_part](size_t offset) -> translated_t {
            return find_longest_part(translate_nucleotides_to_amino_acids(sequence.raw_sequence, offset), offset);
        };

        std::array<translated_t, 3> translated;
        ranges::transform(ranges::views::iota(0UL, translated.size()), std::begin(translated), transformation);

        const auto longest_translated =
            std::max_element(std::begin(translated), std::end(translated), [](const auto& e1, const auto& e2) { return e1.longest.size() < e2.longest.size(); });
        if (longest_translated->longest.size() >= MINIMUM_SEQUENCE_AA_LENGTH)
            sequence.sequence.set(longest_translated->longest, sequence.raw_sequence.substr(longest_translated->offset));
    }

    if (!sequence.sequence.is_translated())
        sequence.issues.set(issue::not_translated);

    aa_trim_absent(sequence, messages);

    return sequence.sequence.is_translated();

} // ae::sequences::translate

// ======================================================================

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

static const std::unordered_map<std::string_view, char, ae::string_hash_for_unordered_map, std::equal_to<>> CODON_TO_PROTEIN = {
    {"UGC", 'C'}, {"GTA", 'V'}, {"GTG", 'V'}, {"CCT", 'P'}, {"CUG", 'L'}, {"AGG", 'R'}, {"CTT", 'L'}, {"CUU", 'L'},
    {"CTG", 'L'}, {"GCU", 'A'}, {"CCG", 'P'}, {"AUG", 'M'}, {"GGC", 'G'}, {"UUA", 'L'}, {"GAG", 'E'}, {"UGG", 'W'},
    {"UUU", 'F'}, {"UUG", 'L'}, {"ACU", 'T'}, {"TTA", 'L'}, {"AAT", 'N'}, {"CGU", 'R'}, {"CCA", 'P'}, {"GCC", 'A'},
    {"GCG", 'A'}, {"TTG", 'L'}, {"CAT", 'H'}, {"AAC", 'N'}, {"GCA", 'A'}, {"GAU", 'D'}, {"UAU", 'Y'}, {"CAC", 'H'},
    {"AUA", 'I'}, {"GUC", 'V'}, {"TCG", 'S'}, {"GGG", 'G'}, {"AGC", 'S'}, {"CTA", 'L'}, {"GCT", 'A'}, {"CCC", 'P'},
    {"ACC", 'T'}, {"GAT", 'D'}, {"TCC", 'S'}, {"UAC", 'Y'}, {"CAU", 'H'}, {"UCG", 'S'}, {"CAA", 'Q'}, {"UCC", 'S'},
    {"AGU", 'S'}, {"TTT", 'F'}, {"ACA", 'T'}, {"ACG", 'T'}, {"CGC", 'R'}, {"TGT", 'C'}, {"CAG", 'Q'}, {"GUA", 'V'},
    {"GGU", 'G'}, {"AAG", 'K'}, {"AGA", 'R'}, {"ATA", 'I'}, {"TAT", 'Y'}, {"UCU", 'S'}, {"TCA", 'S'}, {"GAA", 'E'},
    {"AGT", 'S'}, {"TCT", 'S'}, {"ACT", 'T'}, {"CGA", 'R'}, {"GGT", 'G'}, {"TGC", 'C'}, {"UGU", 'C'}, {"CUC", 'L'},
    {"GAC", 'D'}, {"UUC", 'F'}, {"GTC", 'V'}, {"ATT", 'I'}, {"TAC", 'Y'}, {"CUA", 'L'}, {"TTC", 'F'}, {"GTT", 'V'},
    {"UCA", 'S'}, {"AUC", 'I'}, {"GGA", 'G'}, {"GUG", 'V'}, {"GUU", 'V'}, {"AUU", 'I'}, {"CGT", 'R'}, {"CCU", 'P'},
    {"ATG", 'M'}, {"AAA", 'K'}, {"TGG", 'W'}, {"CGG", 'R'}, {"AAU", 'N'}, {"CTC", 'L'}, {"ATC", 'I'},
    {"TAA", '*'}, {"UAA", '*'}, {"TAG", '*'}, {"UAG", '*'}, {"TGA", '*'}, {"UGA", '*'}, {"TAR", '*'}, {"TRA", '*'}, {"UAR", '*'}, {"URA", '*'},
};

#pragma GCC diagnostic pop

std::string ae::sequences::translate_nucleotides_to_amino_acids(std::string_view nucleotides, size_t offset)
{
    using diff_t = decltype(CODON_TO_PROTEIN)::difference_type;

    std::string result((nucleotides.size() - offset) / 3 + 1, '-');
    auto result_p = result.begin();
    for (auto off = offset; off < (nucleotides.size() - 2); off += 3, ++result_p) {
        if (const auto it = CODON_TO_PROTEIN.find(std::string_view(nucleotides.data() + static_cast<diff_t>(off), 3)); it != CODON_TO_PROTEIN.end())
            *result_p = it->second;
        else
            *result_p = 'X';
    }
    result.resize(static_cast<size_t>(result_p - result.begin()));
    return result;

} // ae::sequences::translate_nucleotides_to_amino_acids

// ----------------------------------------------------------------------

void ae::sequences::aa_trim_absent(RawSequence& sequence, Messages& messages)
{
    if (sequence.sequence.is_translated()) {
        // remove trailing X and - in aa
        if (const auto found = sequence.sequence.aa->find_last_not_of("X-"); found != std::string::npos)
            sequence.sequence.erase_aa(found + 1);
        else
            messages.add(Message::invalid_sequence, sequence.name, "just X and - in AA sequence");

        // remove leading X and -
        if (const auto found = sequence.sequence.aa->find_first_not_of("X-"); found > 0 && found != std::string::npos)
            sequence.sequence.erase_aa(0, found);
    }

} // aa_trim_absent


// ----------------------------------------------------------------------
