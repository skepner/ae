#pragma once

#include <string>
#include <vector>

#include "ext/fmt.hh"
#include "ext/string.hh"
#include "utils/string.hh"
#include "utils/messages.hh"

// ======================================================================

namespace ae::virus::passage
{
    struct deconstructed_t
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
            bool good() const { return !name.empty() && (name == "OR" || !count.empty()); }
        };

        std::vector<element_t> elements{};
        std::string date{};

        constexpr deconstructed_t() = default;
        constexpr deconstructed_t(int) : deconstructed_t() {} // to support lexy::fold_inplace in passage-parse.cc
        deconstructed_t(const std::vector<element_t>& a_elements, const std::string& a_date) : elements{a_elements}, date{a_date} {}
        // deconstructed_t(std::string_view not_parsed) : elements{element_t{.name{not_parsed}}} {}

        bool operator==(const deconstructed_t& rhs) const = default;
        auto operator<=>(const deconstructed_t& rhs) const { return construct() <=> rhs.construct(); }

        bool empty() const { return elements.empty(); }
        const element_t& last() const { return elements.back(); }
        bool egg() const { return !empty() && last().egg(); }
        bool cell() const { return !empty() && last().cell(); }

        enum class with_date { no, yes };

        std::string construct(with_date wd = with_date::yes) const
        {
            std::string result;
            for (const auto& elt : elements)
                result.append(elt.construct(true));
            if (wd == with_date::yes && !date.empty())
                result.append(fmt::format(" ({})", date));
            return result;
        }

        auto& uppercase()
        {
            for (auto& elt : elements) {
                string::uppercase_in_place(elt.name);
                string::uppercase_in_place(elt.count);
            }
            return *this;
        }

        bool good() const
        {
            return !elements.empty() && elements.front().good() && std::count_if(std::begin(elements), std::end(elements), [](const auto& elt) { return !elt.good() || elt.count[0] == '?'; }) < 2;
        }
    };

    // ----------------------------------------------------------------------

    class parse_settings
    {
      public:
        enum class tracing { no, yes };

        parse_settings(tracing a_trace = tracing::no) : tracing_{a_trace} {}

        constexpr bool trace() const { return tracing_ == tracing::yes; }

      private:
        tracing tracing_{tracing::no};
    };

    deconstructed_t parse(std::string_view source, const parse_settings& settings, Messages& messages, const MessageLocation& location);

    inline deconstructed_t parse(std::string_view source)
    {
        Messages messages;
        return parse(source, parse_settings{}, messages, MessageLocation{});
    }

} // namespace ae::virus::passage

// ----------------------------------------------------------------------

namespace ae::virus
{
    class Passage
    {
      public:
        enum class parse { no, yes };

        Passage() = default;
        Passage(const Passage&) = default;
        Passage(Passage&&) = default;
        explicit Passage(std::string_view src, parse pars = parse::yes)
        {
            if (pars == parse::yes)
                deconstructed_ = passage::parse(src);
            else
                deconstructed_.elements.push_back({.name{src}});
        }

        Passage& operator=(const Passage&) = default;
        Passage& operator=(Passage&&) = default;

        bool operator==(const Passage& rhs) const = default;
        auto operator<=>(const Passage& rhs) const { return static_cast<std::string>(*this) <=> static_cast<std::string>(rhs); }

        bool good() const { return deconstructed_.good(); }
        bool empty() const { return deconstructed_.empty(); }
        bool is_egg() const { return deconstructed_.egg(); }
        bool is_cell() const { return deconstructed_.cell(); }
        std::string without_date() const { return deconstructed_.construct(passage::deconstructed_t::with_date::no); }
        operator std::string() const { return deconstructed_.construct(); }
        std::string to_string() const { return deconstructed_.construct(); }
        size_t size() const { return deconstructed_.construct().size(); }

        // std::string_view last_number() const; // E2/E3 -> 3, X? -> ?
        // std::string_view last_type() const; // MDCK3/SITA1 -> SIAT

        std::string_view passage_type() const
        {
            using namespace std::string_view_literals;
            if (is_egg())
                return "egg"sv;
            else
                return "cell"sv;
        }

        // size_t find(std::string_view look_for) const { return get().find(look_for); }
        // bool search(const std::regex& re) const { return std::regex_search(get(), re); }


      private:
        passage::deconstructed_t deconstructed_{};
    };
} // namespace ae::virus

template <> struct fmt::formatter<ae::virus::Passage> : public fmt::formatter<std::string>
{
    template <typename FormatContext> auto format(const ae::virus::Passage& ts, FormatContext& ctx) { return fmt::formatter<std::string>::format(static_cast<std::string>(ts), ctx); }
};

// ======================================================================
