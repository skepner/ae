#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>

#include "ext/compare.hh"
#include "utils/named-type.hh"
#include "sequences/pos.hh"

// ======================================================================

namespace ae::sequences
{
    template <typename Tag> class basic_sequence_t : public named_string_t<std::string, Tag>
    {
      public:
        using named_string_t<std::string, Tag>::named_string_t;
        using named_string_t<std::string, Tag>::operator=;

        template <std::integral Size> void remove_prefix(Size offset) { this->get().erase(0, static_cast<size_t>(offset)); }
        template <std::integral Size> void add_prefix(Size size, char symbol = 'X') { this->get().insert(0, static_cast<size_t>(size), symbol); }

        void truncate(size_t size) { this->get().erase(size); }

        /*constexpr*/ char operator[](pos0_t pos0) const noexcept { return *pos0 < this->size() ? this->get().operator[](*pos0) : ' '; }
        /*constexpr*/ std::string_view substr(pos0_t pos0, size_t size) const { return std::string_view{this->get()}.substr(pos0.get(), size); }
        /*constexpr*/ std::string_view at_end(size_t size) const
        {
            if (const auto pos = this->get().size() - size; pos < this->get().size()) // protect from pos underflow
                return std::string_view{this->get()}.substr(pos);
            else
                return {};
        }

        size_t number_of_x() const { return std::count(std::begin(this->get()), std::end(this->get()), 'X'); }
        size_t number_of_deletions() const { return std::count(std::begin(this->get()), std::end(this->get()), '-'); }

        constexpr pos0_t size() const { return pos0_t{named_string_t<std::string, Tag>::size()}; }
    };

    using sequence_nuc_t = basic_sequence_t<struct sequence_nuc_t_tag>;
    using sequence_aa_t = basic_sequence_t<struct sequence_aa_t_tag>;

    // ----------------------------------------------------------------------

    template <typename Tag> class indexed
    {
      public:
        class iterator
        {
          public:
            iterator(const basic_sequence_t<Tag>& seq, pos0_t pos) : seq_{seq}, pos_{pos} {}
            bool operator==(const iterator& rhs) { return pos_ == rhs.pos_; }
            std::pair<pos1_t, char> operator*() { return {pos1_t{pos_}, seq_[pos_]}; }
            iterator& operator++() { ++pos_; return *this; }

          private:
            const basic_sequence_t<Tag>& seq_;
            pos0_t pos_;
        };

        indexed(const basic_sequence_t<Tag>& seq) : seq_{seq} {}

        iterator begin() const { return iterator{seq_, pos0_t{0}}; }
        iterator end() const { return iterator{seq_, pos0_t{seq_.size()}}; }

      private:
        const basic_sequence_t<Tag>& seq_;
    };

    template <typename Tag> indexed(const basic_sequence_t<Tag>& seq) -> indexed<Tag>;

    // ----------------------------------------------------------------------

    struct sequence_pair_t
    {
        sequence_aa_t aa;
        sequence_nuc_t nuc;

        enum class truncate_nuc { no, yes };

        void set(std::string_view a_aa, std::string_view a_nuc, truncate_nuc tn = truncate_nuc::yes)
        {
            const auto expected_nuc = a_aa.size() * 3;
            if (expected_nuc < a_nuc.size() && tn == truncate_nuc::yes)
                a_nuc.remove_suffix(a_nuc.size() - expected_nuc);
            else if (expected_nuc != a_nuc.size())
                throw std::runtime_error{fmt::format("cannot set sequence_pair_t, length mismatch: aa:{} nuc:{} (expected:{})", a_aa.size(), a_nuc.size(), expected_nuc)};
            aa = sequence_aa_t{a_aa};
            nuc = sequence_nuc_t{a_nuc};
        }

        bool is_translated() const { return !aa.empty(); }

        void erase_aa(pos0_t pos, size_t count = std::string::npos)
        {
            aa.get().erase(*pos, count);
            const auto count_nuc = count == std::string::npos ? count : count * 3;
            nuc.get().erase(*pos * 3, count_nuc);
        }

        void add_prefix_aa(pos0_t size, char symbol = 'X')
        {
            aa.add_prefix(*size, symbol);
            nuc.add_prefix(*size * 3, symbol);
        }
        void remove_prefix_aa(pos0_t size)
        {
            aa.remove_prefix(*size);
            nuc.remove_prefix(*size * 3);
        }
    };

    // ----------------------------------------------------------------------

    struct insertion_t
    {
        pos0_t pos{0};
        std::string insertion{};
        auto operator<=>(const insertion_t&) const = default;
    };

    using insertions_t = std::vector<insertion_t>;

} // namespace ae::sequences

// ----------------------------------------------------------------------

// used for json output -> [pos, insertion]
template <> struct fmt::formatter<ae::sequences::insertion_t> : fmt::formatter<ae::fmt_helper::default_formatter> {
     auto format(const ae::sequences::insertion_t& value, format_context& ctx) const
    {
        return fmt::format_to(ctx.out(), "[{}, \"{}\"]", value.pos, value.insertion);
    }
};


// ======================================================================
