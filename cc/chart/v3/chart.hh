#pragma once

#include "ext/filesystem.hh"
#include "chart/v3/info.hh"
#include "chart/v3/antigens.hh"
#include "chart/v3/titers.hh"
#include "chart/v3/projections.hh"
#include "chart/v3/styles.hh"

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
        Titers& titers() { return titers_; }
        const Titers& titers() const { return titers_; }
        Projections& projections() { return projections_; }
        const Projections& projections() const { return projections_; }
        Styles& styles() { return styles_; }
        const Styles& styles() const { return styles_; }

        void write(const std::filesystem::path& filename) const;

      private:
        Info info_{};
        Antigens antigens_{};
        Sera sera_{};
        Titers titers_{};
        Projections projections_{};
        Styles styles_{};

        void read(const std::filesystem::path& filename);
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
