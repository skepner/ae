#pragma once

#include <string>
#include <vector>

#include "ext/fmt.hh"
#include "utils/string.hh"

// ======================================================================

namespace ae::virus::passage
{
    struct passage_deconstructed_t
    {
        struct element_t
        {
            std::string name{};
            std::string count{}; // if count empty, name did not parsed
            bool new_lab{false};

            bool operator==(const element_t& rhs) const = default;

            std::string construct(bool add_new_lab_separator) const
            {
                std::string result;
                result.append(name);
                result.append(count);
                if (!count.empty() && add_new_lab_separator && new_lab)
                    result.append(1, '/');
                return result;
            }

            bool egg() const { return name == "E" || name == "SPFCE"; }
            bool cell() const { return name == "MDCK" || name == "SIAT" || name == "HCK" || name == "SPFCK"; }
        };

        std::vector<element_t> elements{};
        std::string date{};

        constexpr passage_deconstructed_t() = default;
        constexpr passage_deconstructed_t(int) : passage_deconstructed_t() {} // to support lexy::fold_inplace in passage-parse.cc
        passage_deconstructed_t(const std::vector<element_t>& a_elements, const std::string& a_date) : elements{a_elements}, date{a_date} {}

        bool operator==(const passage_deconstructed_t& rhs) const = default;

        bool empty() const { return elements.empty(); }
        const element_t& last() const { return elements.back(); }
        bool egg() const { return !empty() && last().egg(); }
        bool cell() const { return !empty() && last().cell(); }

        std::string construct() const
        {
            std::string result;
            for (const auto& elt : elements)
                result.append(elt.construct(true));
            if (!date.empty())
                result.append(fmt::format(" ({})", date));
            return result;
        }

        bool good() const
        {
            return !elements.empty() && !elements.front().name.empty() && !elements.front().count.empty() &&
                   std::count_if(std::begin(elements), std::end(elements), [](const auto& elt) { return elt.count.empty() || elt.count[0] == '?'; }) < 2;
        }
    };

} // namespace ae::virus::passage

// ======================================================================
