#pragma once

#include <string>

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

        /*constexpr*/ char operator[](pos0_t pos0) const noexcept { return *pos0 < this->size() ? this->get().operator[](*pos0) : ' '; }
        /*constexpr*/ std::string_view substr(pos0_t pos0, size_t size) const { return std::string_view{this->get()}.substr(pos0.get(), size); }
    };

    // class sequence_nuc_t : public basic_sequence_t<struct sequence_nuc_t_tag>
    // {
    //   public:
    //     using Tag = struct sequence_nuc_t_tag;
    //     using basic_sequence_t<Tag>::named_string_t;
    //     using basic_sequence_t<Tag>::operator=;

    // };

    using sequence_nuc_t = basic_sequence_t<struct sequence_nuc_t_tag>;
    using sequence_aa_t = basic_sequence_t<struct sequence_aa_t_tag>;
}

// ======================================================================
