#pragma once

#include "utils/string.hh"

// ----------------------------------------------------------------------

namespace acmacs
{
    template <typename DataFormat> class DataFormatter
    {
     public:
       template <typename F> static inline std::string& first_field(std::string& target, F&& field)
       {
           target.append(DataFormat::field_prefix);
           target.append(DataFormat::prepare_field(std::forward<F>(field)));
           target.append(DataFormat::field_suffix);
           return target;
       }

       template <typename F> static inline std::string& second_field(std::string& target, F&& field)
       {
           target.append(DataFormat::field_separator);
           return first_field(target, std::forward<F>(field));
       }

       static inline std::string& end_of_record(std::string& target)
       {
           target.append(DataFormat::record_separator);
           return target;
       }

    }; // class DataFormatter<DataFormat>

    struct DataFormatSpaceSeparated
    {
        constexpr static const char* field_prefix = "";
        constexpr static const char* field_suffix = "";
        constexpr static const char* field_separator = " ";
        constexpr static const char* record_separator = "\n";

        template <typename F> static inline std::string prepare_field(F&& field) { return ae::string::replace_spaces(fmt::format("{}", field), '_'); }
    };

    struct DataFormatCSV
    {
        constexpr static const char* field_prefix = "\"";
        constexpr static const char* field_suffix = "\"";
        constexpr static const char* field_separator = ",";
        constexpr static const char* record_separator = "\n";

        template <typename F> static inline std::string prepare_field(F&& field) { return fmt::format("{}", field); }
    };

    using DataFormatterSpaceSeparated = DataFormatter<DataFormatSpaceSeparated>;
    using DataFormatterCSV = DataFormatter<DataFormatCSV>;

} // namespace acmacs

// ----------------------------------------------------------------------
