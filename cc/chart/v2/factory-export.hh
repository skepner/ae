#pragma once

#include <string>
#include <memory>

// ----------------------------------------------------------------------

namespace ae::chart::v2
{
    class Chart;

    void export_factory(const Chart& chart, std::string_view filename, std::string_view program_name);

    enum class export_format { ace, save, text, text_table };
    std::string export_factory(const Chart& chart, export_format format, std::string_view program_name);

} // namespace ae::chart::v2

// ----------------------------------------------------------------------
