#pragma once

#include "ext/filesystem.hh"
#include "ext/xlnt.hh"
#include "utils/float.hh"
#include "whocc/xlsx/sheet.hh"

// ----------------------------------------------------------------------

namespace ae::xlsx::inline v1
{
    namespace xlnt
    {
        class Doc;

        class Sheet : public ae::xlsx::Sheet
        {
          public:
            Sheet(::xlnt::worksheet&& src) : sheet_{std::move(src)}, number_of_rows_{sheet_.highest_row()}, number_of_columns_{sheet_.highest_column().index}
            {
                if (number_of_columns_ > ncol_t{0} && number_of_rows_ > nrow_t{0}) {
                    // remove last empty columns
                    const auto is_empty_col = [this](ncol_t col) {
                        for (nrow_t row{0}; row < number_of_rows_; ++row) {
                            if (!is_empty(cell(row, col)))
                                return false;
                        }
                        return true;
                    };

                    for (ncol_t col{number_of_columns_ - ncol_t{1}}; col > ncol_t{0}; --col) {
                        if (!is_empty_col(col))
                            break;
                        --number_of_columns_;
                    }

                    // remove last empty rows
                    const auto is_empty_row = [this](nrow_t row) {
                        for (ncol_t col{0}; col < number_of_columns_; ++col) {
                            if (!is_empty(cell(row, col)))
                                return false;
                        }
                        return true;
                    };

                    for (nrow_t row{number_of_rows_ - nrow_t{1}}; row > nrow_t{0}; --row) {
                        if (!is_empty_row(row))
                            break;
                        --number_of_rows_;
                    }
                }
            }

            std::string name() const override { return sheet_.title(); }
            xlsx::nrow_t number_of_rows() const override { return number_of_rows_; }
            xlsx::ncol_t number_of_columns() const override { return number_of_columns_; }

            static inline std::chrono::year_month_day make_date(const ::xlnt::datetime& dt, xlsx::nrow_t /*row*/, xlsx::ncol_t /*col*/)
            {
                // if (dt.hour || dt.minute || dt.second || dt.microsecond)
                //     AD_WARNING("xlnt datetime at {}{} contains time: {}", col, row, dt.to_string());
                return std::chrono::year{dt.year} / std::chrono::month{static_cast<unsigned>(dt.month)} / dt.day;
            }

            static inline bool is_date(const ::xlnt::cell& cell)
            {
                try {
                    return cell.is_date();
                }
                catch (...) {
                    return false; // xlnt throws when xlsx format is unsupported in some aspect
                }
            }

            ae::xlsx::cell_t cell(xlsx::nrow_t row, xlsx::ncol_t col) const override // row and col are zero based
            {
                const ::xlnt::cell_reference ref{static_cast<::xlnt::column_t::index_t>(*col + 1), static_cast<::xlnt::row_t>(*row + 1)};
                if (!sheet_.has_cell(ref))
                    return ae::xlsx::cell::empty{};

                const auto cell = sheet_.cell(ref);
                switch (cell.data_type()) { // ~/ae/subprojects/xlnt-1.5.0/include/xlnt/cell/cell_type.hpp
                    case ::xlnt::cell_type::empty:
                        return ae::xlsx::cell::empty{};
                    case ::xlnt::cell_type::boolean:
                        return cell.value<bool>();
                    case ::xlnt::cell_type::inline_string:
                    case ::xlnt::cell_type::shared_string:
                    case ::xlnt::cell_type::formula_string:
                        if (const auto val = cell.value<std::string>(); !val.empty())
                            return val;
                        else
                            return ae::xlsx::cell::empty{};
                    case ::xlnt::cell_type::number:
                        if (is_date(cell))
                            return make_date(cell.value<::xlnt::datetime>(), row, col);
                        else if (const auto vald = cell.value<double>(); !float_equal(vald, std::round(vald)))
                            return vald;
                        else
                            return static_cast<long>(cell.value<long long>());
                    case ::xlnt::cell_type::date:
                        return make_date(cell.value<::xlnt::datetime>(), row, col);
                    case ::xlnt::cell_type::error:
                        return ae::xlsx::cell::error{};
                }
                return ae::xlsx::cell::empty{};
            }

            // ae::xlsx::cell_spans_t cell_spans(nrow_t row, ncol_t col) const override
            // {
            //     const ::xlnt::cell_reference ref{static_cast<::xlnt::column_t::index_t>(col + 1), static_cast<::xlnt::row_t>(row + 1)};
            //     if (!sheet_.has_cell(ref))
            //         return {};
            //     ae::xlsx::cell_spans_t spans;
            //     // const auto cell = sheet_.cell(ref);
            //     // if (const auto fill = cell.fill(); fill.type() == ::xlnt::fill_type::pattern) {
            //     //     const auto fill_pattern = fill.pattern_fill();

            //     //     const auto get_color = [](const auto& col) -> std::string {
            //     //         if (col.is_set() && col.get().type() == ::xlnt::color_type::rgb) {
            //     //             const auto rgb = col.get().rgb();
            //     //             return fmt::format("#{:02x}{:02x}{:02x}", rgb.red(), rgb.green(), rgb.blue());
            //     //         }
            //     //         else
            //     //             return {};
            //     //     };

            //     //     const std::string foreground = get_color(fill_pattern.foreground()), background = get_color(fill_pattern.background());
            //     //     if (!foreground.empty() || !background.empty())
            //     //         spans.push_back(ae::xlsx::cell_span_t{0, 1, foreground, background});
            //     // }

            //     // size_t first{0};
            //     // for (const auto& run : cell.value<::xlnt::rich_text>().runs()) {
            //     //     if (run.second.is_set()) {
            //     //         std::string color_value;
            //     //         if (run.second.get().has_color()) {
            //     //             if (const auto color = run.second.get().color(); color.type() == ::xlnt::color_type::rgb)
            //     //                 color_value = color.rgb().hex_string();
            //     //         }
            //     //         spans.push_back(ae::xlsx::cell_span_t{first, run.first.size(), color_value});
            //     //     }
            //     //     first += run.first.size();
            //     // }
            //     return spans;
            // }

          private:
            ::xlnt::worksheet sheet_;
            xlsx::nrow_t number_of_rows_;
            xlsx::ncol_t number_of_columns_;
        };

        class Doc
        {
          public:
            Doc(const std::filesystem::path& filename) : workbook_{::xlnt::path{std::string{filename}}} {}

            size_t number_of_sheets() const { return workbook_.sheet_count(); }
            std::shared_ptr<ae::xlsx::Sheet> sheet(size_t sheet_no) { return std::make_shared<Sheet>(workbook_.sheet_by_index(sheet_no)); }

          private:
            ::xlnt::workbook workbook_;
        };

    } // namespace xlnt

} // namespace ae::xlsx::inline v1

// ----------------------------------------------------------------------
