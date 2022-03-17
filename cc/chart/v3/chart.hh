#pragma once

#include "ext/filesystem.hh"
#include "chart/v3/info.hh"
#include "chart/v3/antigens.hh"
#include "chart/v3/titers.hh"
#include "chart/v3/projections.hh"
#include "chart/v3/styles.hh"
#include "chart/v3/legacy-plot-spec.hh"
#include "chart/v3/optimize-options.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Error : public std::runtime_error
    {
      public:
        template <typename... Args> Error(fmt::format_string<Args...> format, Args&&... args) : std::runtime_error{fmt::format("[chart] {}", fmt::format(format, std::forward<Args>(args)...))} {}
    };

    // ----------------------------------------------------------------------

    class Chart
    {
      public:
        Chart() = default;
        Chart(const std::filesystem::path& filename) { read(filename); }

        Chart(const Chart&) = default;
        Chart(Chart&&) = default;
        Chart& operator=(const Chart&) = default;
        Chart& operator=(Chart&&) = default;

        const Info& info() const { return info_; }
        Info& info() { return info_; }

        Antigens& antigens() { return antigens_; }
        const Antigens& antigens() const { return antigens_; }
        Sera& sera() { return sera_; }
        const Sera& sera() const { return sera_; }
        template <typename AgSr> AgSr& antigens_sera()
        {
            if constexpr (std::is_same_v<AgSr, Antigens>)
                return antigens_;
            else
                return sera_;
        }
        Titers& titers() { return titers_; }
        const Titers& titers() const { return titers_; }
        Projections& projections() { return projections_; }
        const Projections& projections() const { return projections_; }
        Styles& styles() { return styles_; }
        const Styles& styles() const { return styles_; }
        legacy::PlotSpec& legacy_plot_spec() { return legacy_plot_spec_; }
        const legacy::PlotSpec& legacy_plot_spec() const { return legacy_plot_spec_; }

        void write(const std::filesystem::path& filename) const;

        std::string name(std::optional<projection_index> aProjectionNo = std::nullopt) const;
        std::string name_for_file() const;

        point_index number_of_points() const { return point_index{antigens().size().get() + sera().size().get()}; }
        class column_bases column_bases(minimum_column_basis mcb) const;
        class column_bases forced_column_bases() const;

        void relax(number_of_optimizations_t number_of_optimizations, minimum_column_basis mcb, number_of_dimensions_t number_of_dimensions, const optimization_options& options,
                   const disconnected_points& disconnected = disconnected_points{}, const unmovable_points& unmovable = unmovable_points{});
        void relax_incremental(projection_index source_projection_no, number_of_optimizations_t number_of_optimizations, const optimization_options& options,
                               const disconnected_points& disconnected = disconnected_points{}, const unmovable_points& unmovable = unmovable_points{});

        void combine_projections(const Chart& merge_in);

        void duplicates_distinct();

      private:
        Info info_{};
        Antigens antigens_{};
        Sera sera_{};
        Titers titers_{};
        Projections projections_{};
        Styles styles_{};
        legacy::PlotSpec legacy_plot_spec_{};

        void read(const std::filesystem::path& filename);
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
