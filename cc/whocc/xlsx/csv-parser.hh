#pragma once

#include "ext/filesystem.hh"
#include "whocc/xlsx/sheet.hh"

// ----------------------------------------------------------------------

namespace ae::xlsx::inline v1
{
    namespace csv
    {
        class Sheet : public ae::xlsx::Sheet
        {
          public:
            Sheet(const std::filesystem::path& filename);

            xlsx::nrow_t number_of_rows() const override { return xlsx::nrow_t{data_.size()}; }
            xlsx::ncol_t number_of_columns() const override { return number_of_columns_; }
            std::string name() const override { return {}; }
            ae::xlsx::cell_t cell(xlsx::nrow_t row, xlsx::ncol_t col) const override { return data_.at(*row).at(*col); } // row and col are zero based

          private:
            std::vector<std::vector<ae::xlsx::cell_t>> data_;
            xlsx::ncol_t number_of_columns_{0};
        };

        class Doc
        {
          public:
            Doc(const std::filesystem::path& filename) : sheet_{std::make_shared<Sheet>(filename)} {}

            size_t number_of_sheets() const { return 1; }
            std::shared_ptr<ae::xlsx::Sheet> sheet(size_t /*sheet_no*/) { return sheet_; }

          private:
            std::shared_ptr<Sheet> sheet_;
        };

    } // namespace csv

} // namespace ae::xlsx::inline v1

// ----------------------------------------------------------------------
