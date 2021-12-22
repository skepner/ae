#include "utils/file.hh"
#include "utils/string.hh"
#include "chart/v2/factory-export.hh"
#include "chart/v2/ace-export.hh"
#include "chart/v2/lispmds-export.hh"
#include "chart/v2/text-export.hh"
#include "chart/v2/verify.hh"

// ----------------------------------------------------------------------

std::string ae::chart::v2::export_factory(const Chart& chart, export_format format, std::string_view program_name)
{
    switch (format) {
      case export_format::ace:
          return export_ace(chart, program_name, 1);
      case export_format::save:
          return export_lispmds(chart, program_name);
      case export_format::text:
          return export_text(chart);
      case export_format::text_table:
          return export_table_to_text(chart);
    }
    return {};

} // ae::chart::v2::export_factory

// ----------------------------------------------------------------------

inline bool endswith(std::string_view source, std::string_view suffix) { return source.size() >= suffix.size() && source.substr(source.size() - suffix.size()) == suffix; }

void ae::chart::v2::export_factory(const Chart& chart, std::string_view filename, std::string_view program_name)
{
    using namespace std::string_view_literals;

    std::string data;
    if (endswith(filename, ".ace"sv))
        data = export_factory(chart, export_format::ace, program_name);
    else if (endswith(filename, ".save"sv) || endswith(filename, ".save.xz"sv) || endswith(filename, ".save.gz"sv))
        data = export_factory(chart, export_format::save, program_name);
    else if (endswith(filename, ".table.txt"sv) || endswith(filename, ".table.txt.xz"sv) || endswith(filename, ".table.txt.gz"sv) || endswith(filename, ".table"sv) || endswith(filename, ".table.xz"sv) || endswith(filename, ".table.gz"sv))
        data = export_factory(chart, export_format::text_table, program_name);
    else if (endswith(filename, ".txt"sv) || endswith(filename, ".txt.xz"sv) || endswith(filename, ".txt.gz"sv))
        data = export_factory(chart, export_format::text, program_name);
    else
        throw import_error{fmt::format("[ae::chart::v2::export_factory]: cannot infer export format from extension of {}", filename)};

    if (data.empty())
        throw export_error{fmt::format("No data to write to {}", filename)};

    // Timeit ti_file(fmt::format("writing {}: ", filename), report);
    ae::file::write(filename, data, endswith(filename, ".ace"sv) ? ae::file::force_compression::yes : ae::file::force_compression::no);

} // ae::chart::v2::export_factory

// ----------------------------------------------------------------------
