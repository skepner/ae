#pragma once

#include "utils/log.hh"
#include "utils/collection.hh"
#include "sequences/clades.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class SemanticAttributes
    {
      public:
        bool operator==(const SemanticAttributes&) const = default;

        bool empty() const { return data_.empty(); }

        // sequences::clades_t clades;

        auto& data() { return data_; }

        void clades(const sequences::clades_t& clades) { AD_WARNING("SemanticAttributes::clades(clades) not implemented"); }
        void add_clade(std::string_view clade) { AD_WARNING("SemanticAttributes::add_clade(std::string_view) not implemented");; }
        bool has_clade(std::string_view clade) const { AD_WARNING("SemanticAttributes::has_clade(std::string_view) not implemented"); return false; }

      private:
        DynamicCollection data_;
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
