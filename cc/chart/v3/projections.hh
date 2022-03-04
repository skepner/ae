#pragma once

#include <vector>

#include "chart/v3/index.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Chart;

    class Projection
    {
      public:
        Projection() = default;
        Projection(const Projection&) = default;
        Projection(Projection&&) = default;
        Projection& operator=(const Projection&) = default;
        Projection& operator=(Projection&&) = default;
    };

    // ----------------------------------------------------------------------

    class Projections
    {
      public:
        Projections() = default;
        Projections(const Projections&) = default;
        Projections(Projections&&) = default;
        Projections& operator=(const Projections&) = default;
        Projections& operator=(Projections&&) = default;

        bool empty() const { return data_.empty(); }
        size_t size() const { return data_.size(); }
        const Projection& operator[](projection_index aIndex) const { return data_[*aIndex]; }
        Projection& operator[](projection_index aIndex) { return data_[*aIndex]; }
        const Projection& best() const { return operator[](projection_index{0}); }
        auto begin() const { return data_.begin(); }
        auto end() const { return data_.end(); }
        Projection& add() { return data_.emplace_back(); }
        // std::string make_info(size_t max_number_of_projections_to_show = 20) const;

      private:
        std::vector<Projection> data_;
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v3::Projection> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::chart::v3::Projection& projection, FormatCtx& ctx) const
        {
            format_to(ctx.out(), "PROJECTION");
            return ctx.out();
        }
};

// ----------------------------------------------------------------------
