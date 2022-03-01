#pragma once

#include "virus/virus.hh"
#include "virus/type-subtype.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Assay : public ae::named_string_t<std::string, struct assay_tag>
    {
      public:
        using ae::named_string_t<std::string, struct assay_tag>::named_string_t;
        enum class no_hi { no, yes };

        std::string hi_or_neut(no_hi nh = no_hi::no) const;
        std::string HI_or_Neut(no_hi nh = no_hi::no) const;
        std::string short_name() const;
    };

    // ----------------------------------------------------------------------

    class Info
    {
      public:
        Info() = default;
        Info(const Info&) = delete;
        Info(Info&&) = default;
        Info& operator=(const Info&) = delete;
        Info& operator=(Info&&) = default;

      private:
        ae::virus::virus_t virus_{};
        ae::virus::type_subtype_t type_subtype_{};
        Assay assay_{};

// |             |     | "D" |     |     | str, date YYYYMMDD.NNN           | table/assay date and number (if multiple on that day), e.g. 20160602.002                                                                                       |
// |             |     | "N" |     |     | str                              | user supplied name                                                                                                                                             |
// |             |     | "l" |     |     | str                              | lab                                                                                                                                                            |
// |             |     | "r" |     |     | str                              | RBCs species of HI assay, e.g. "turkey"                                                                                                                        |
// |             |     | "s" |     |     | str                              | subset/lineage, e.g. "2009PDM"                                                                                                                                 |
// |             |     | "T" |     |     | str                              | table type "A[NTIGENIC]" - default, "G[ENETIC]"                                                                                                                |
// |             |     | "S" |     |     | array of key-value pairs         | source table info list, each entry is like "i"                                                                                                                 |
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
