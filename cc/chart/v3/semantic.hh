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

        // sequences::clades_t clades;

        const auto& data() const { return data_; }
        auto& data() { return data_; }

        void clades(const sequences::clades_t& clades)
        {
            data_["C"] = ae::dynamic::array{};
            for (const auto& clade : clades)
                data_["C"].add(clade);
        }

        void add_clade(std::string_view clade)
        {
            auto& clades = data_["C"];
            if (clades.is_null())
                clades = ae::dynamic::array{};
            clades.add(clade);
        }

        bool has_clade(std::string_view clade) const
        {
            return data_["C"].contains(clade);
        }

      private:
        DynamicCollection data_;
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
