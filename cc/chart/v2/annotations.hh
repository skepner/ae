#pragma once

#include <numeric>

#include "utils/named-vector.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v2
{
    class Annotations : public ae::named_vector_t<std::string, struct chart_Annotations_tag_t>
    {
      public:
        constexpr static const char* const distinct_label{"DISTINCT"};

        using ae::named_vector_t<std::string, struct chart_Annotations_tag_t>::named_vector_t;
        explicit Annotations(size_t num_annotations) : ae::named_vector_t<std::string, struct chart_Annotations_tag_t>::named_vector_t(num_annotations) {}
        // Annotations(const rjson::value& src) : acmacs::named_vector_t<std::string, struct chart_Annotations_tag_t>::named_vector_t(src.size()) { rjson::copy(src, begin()); }

        bool distinct() const { return contains(distinct_label); }
        void set_distinct() { insert_if_not_present(distinct_label); }

        size_t total_length() const
        {
            return std::accumulate(begin(), end(), size_t{0}, [](size_t sum, const auto& element) { return sum + element.size(); });
        }

        // returns if annotations of antigen and serum matches (e.g. ignores CONC for serum), used for homologous pairs finding
        static bool match_antigen_serum(const Annotations& antigen, const Annotations& serum); // chart.cc

    }; // class Annotations

} // namespace ae::chart::v2

// ----------------------------------------------------------------------

// {} -> ["DISTINCT", "NEW"]
// {: } -> joined with space: "DISTINCT NEW"
// {:/} -> joined with /: "DISTINCT/NEW"

template <> struct fmt::formatter<ae::chart::v2::Annotations>
{
    template <typename ParseContext> constexpr auto parse(ParseContext& ctx)
    {
        auto it = ctx.begin();
        if (it != ctx.end() && *it == ':')
            ++it;
        const auto end = std::find(it, ctx.end(), '}');
        if (it != end)
            join_ = *it;
        return end;
    }

    auto format(const ae::chart::v2::Annotations& annotations, format_context& ctx) const
    {
        if (join_) {
            bool put_join{ false };
            for (const auto& ann : annotations) {
                if (put_join)
                    fmt::format_to(ctx.out(), "{:c}", join_);
                else
                    put_join = true;
                fmt::format_to(ctx.out(), "{}", ann);
            }
        }
        else {
            bool put_join{ false };
            fmt::format_to(ctx.out(), "[");
            for (const auto& ann : annotations) {
                if (put_join)
                    fmt::format_to(ctx.out(), ", ");
                else
                    put_join = true;
                fmt::format_to(ctx.out(), "\"{}\"", ann);
            }
            fmt::format_to(ctx.out(), "]");
        }
        return ctx.out();
    }

  private:
    char join_{0};
};

// ----------------------------------------------------------------------
