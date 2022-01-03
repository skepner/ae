#pragma once

#include <variant>
#include <memory>

#include "whocc/xlsx/xlnt.hh"
#include "whocc/xlsx/csv-parser.hh"
#include "whocc/xlsx/error.hh"

// ----------------------------------------------------------------------

namespace ae::xlsx::inline v1
{

    // using XlSheet = xlnt::Sheet;
    using XlDoc = xlnt::Doc;

    // ----------------------------------------------------------------------

    class Doc
    {
      public:
        size_t number_of_sheets() const
        {
            return std::visit([](const auto& ptr) { return ptr->number_of_sheets(); }, doc_);
        }

        std::shared_ptr<Sheet> sheet(size_t sheet_no)
        {
            return std::visit([sheet_no](const auto& ptr) { return ptr->sheet(sheet_no); }, doc_);
        }

        // protected
        Doc(const std::filesystem::path& filename)
        {
            if (const auto ext = filename.extension(); ext == ".csv")
                doc_ = std::make_unique<csv::Doc>(filename);
            else if (ext == ".xlsx")
                doc_ = std::make_unique<XlDoc>(filename);
            else
                throw Error{fmt::format("unsupported suffix in {}", filename)};
        }

      private:
        std::variant<std::unique_ptr<XlDoc>, std::unique_ptr<csv::Doc>> doc_{};

        // friend std::shared_ptr<Doc> open(const std::filesystem::path& filename);
    };

    // ----------------------------------------------------------------------

    inline std::shared_ptr<Doc> open(const std::filesystem::path& filename) { return std::make_shared<Doc>(filename); }

} // namespace ae::xlsx::inline v1

// ----------------------------------------------------------------------
