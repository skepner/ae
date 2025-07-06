#pragma once

#include "ext/range-v3.hh"
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

        constexpr const auto& data() const { return data_; }
        auto& data() { return data_; }

        // ----------------------------------------------------------------------

        void clades(const sequences::clades_t& clades)
        {
            auto& clds = data_.as_array(_clades_key);
            for (const auto& clade : clades)
                clds.add_if_not_present(clade);
        }

        std::vector<std::string_view> clades() const
            {
                return std::visit(
                    []<typename C>(const C& content) -> std::vector<std::string_view> {
                        if constexpr (std::is_same_v<C, ae::dynamic::array>) {
                            std::vector<std::string_view> res(content.size());
                            std::transform(content.begin(), content.end(), res.begin(), [](const auto& val) -> std::string_view { return val.as_string_or_empty(); });
                            return res;
                        }
                        else if constexpr (std::is_same_v<C, ae::dynamic::null>)
                            return {};
                        else
                            throw ae::dynamic::invalid_value{"internal: value of the symantic attribute \"{}\" is not array", _clades_key};
                    },
                    data_[_clades_key].data());
                // const auto& clades = ;
                // if (clades.is_null())
                //     return {};
                // if (!clades.is_array())
            }

        void add_clade(std::string_view clade) { data_.as_array(_clades_key).add_if_not_present(clade); }

        bool has_clade(std::string_view clade) const { return data_[_clades_key].contains(clade); }

        bool has_any_clade_of(const std::vector<std::string>& clades) const
        {
            return ranges::any_of(clades, [this](const auto& clade) { return has_clade(clade); });
        }

        // ----------------------------------------------------------------------

        void vaccine(std::string_view vac) { data_[_vaccine_key] = vac; }

        std::string_view vaccine() const { return data_[_vaccine_key].as_string_or_empty(); }

        // ----------------------------------------------------------------------

        template <typename Val> void set(std::string_view key, Val value) { data_[key] = value; }

        const auto& get(std::string_view key) const { return data_[key]; }

        void remove(std::string_view key) { data_.remove(key); }
        void remove_all() { data_ = DynamicCollection{}; }

      private:
        DynamicCollection data_{};
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v3::SemanticAttributes> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    auto format(const ae::chart::v3::SemanticAttributes& attr, format_context& ctx) const
    {
        return fmt::format_to(ctx.out(), fmt::runtime("{}"), attr.data());
    }
};

// ======================================================================
