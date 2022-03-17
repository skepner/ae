#pragma once

#include <numeric>

#include "utils/named-vector.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Annotations : public ae::named_vector_t<std::string, struct chart_Annotations_tag_t>
    {
      public:
        constexpr static const char* const distinct_label{"DISTINCT"};

        using ae::named_vector_t<std::string, struct chart_Annotations_tag_t>::named_vector_t;
        explicit Annotations(size_t num_annotations) : ae::named_vector_t<std::string, struct chart_Annotations_tag_t>::named_vector_t(num_annotations) {}
        auto operator<=>(const Annotations&) const = default;

        void add(std::string_view value) { insert_if_not_present(value); }
        bool distinct() const { return contains(std::string{distinct_label}); }
        void set_distinct() { insert_if_not_present(std::string{distinct_label}); }

        size_t total_length() const
        {
            return std::accumulate(begin(), end(), size_t{0}, [](size_t sum, const auto& element) { return sum + element.size(); });
        }

        // returns if annotations of antigen and serum matches (e.g. ignores CONC for serum), used for homologous pairs finding
        static bool match(const Annotations& antigen, const Annotations& serum); // antigens.cc

    }; // class Annotations

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

// {} -> ["DISTINCT", "NEW"]
// {: } -> joined with space: "DISTINCT NEW"
// {:/} -> joined with /: "DISTINCT/NEW"

template <> struct fmt::formatter<ae::chart::v3::Annotations>
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

    template <typename FormatContext> constexpr auto format(const ae::chart::v3::Annotations& annotations, FormatContext& ctx)
    {
        if (join_) {
            bool put_join{ false };
            for (const auto& ann : annotations) {
                if (put_join)
                    format_to(ctx.out(), "{:c}", join_);
                else
                    put_join = true;
                format_to(ctx.out(), "{}", ann);
            }
        }
        else {
            bool put_join{ false };
            format_to(ctx.out(), "[");
            for (const auto& ann : annotations) {
                if (put_join)
                    format_to(ctx.out(), ", ");
                else
                    put_join = true;
                format_to(ctx.out(), "\"{}\"", ann);
            }
            format_to(ctx.out(), "]");
        }
        return ctx.out();
    }

  private:
    char join_{0};
};

// ----------------------------------------------------------------------
