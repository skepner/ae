#pragma once

#include "utils/log.hh"
#include "utils/collection.hh"
#include "sequences/clades.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class SemanticAttributes
    {
      private:
        static inline std::string_view _clades_key{"C"};
        static inline std::string_view _vaccine_key{"V"};

      public:
        bool operator==(const SemanticAttributes&) const = default;

        bool empty() const { return data_.empty(); }

        const auto& data() const { return data_; }
        auto& data() { return data_; }

        // ----------------------------------------------------------------------

        void clades(const sequences::clades_t& clades)
        {
            auto& clds = data_.as_array(_clades_key);
            for (const auto& clade : clades)
                clds.add(clade);
        }

        void add_clade(std::string_view clade)
        {
            data_.as_array(_clades_key).add(clade);
        }

        bool has_clade(std::string_view clade) const
        {
            return data_[_clades_key].contains(clade);
        }

        // ----------------------------------------------------------------------

        void vaccine(std::string_view vac)
            {
                data_[_vaccine_key] = vac;
            }

        std::string_view vaccine() const
            {
                return data_[_vaccine_key].as_string_or_empty();
            }

        // ----------------------------------------------------------------------

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
