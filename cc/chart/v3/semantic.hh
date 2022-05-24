#pragma once

#include "utils/log.hh"
#include "utils/collection.hh"
#include "sequences/clades.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class SemanticAttributes
    {
      public:
        bool operator==(const SemanticAttributes&) const = default;

        bool empty() const { return data_.empty(); }

        const auto& data() const { return data_; }
        auto& data() { return data_; }

        void clades(const sequences::clades_t& clades)
        {
            auto& clds = data_.as_array("C");
            for (const auto& clade : clades)
                clds.add(clade);
        }

        void add_clade(std::string_view clade)
        {
            data_.as_array("C").add(clade);
        }

        bool has_clade(std::string_view clade) const
        {
            return data_["C"].contains(clade);
        }

      private:
        DynamicCollection data_{};
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v3::SemanticAttributes> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> constexpr auto format(const ae::chart::v3::SemanticAttributes& attr, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "{}", attr.data());
    }
};

// ======================================================================
