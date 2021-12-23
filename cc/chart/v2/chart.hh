#pragma once

#include <memory>
#include <cmath>
#include <optional>
#include <type_traits>

#include "ad/color.hh"
#include "ad/point-style.hh"
#include "chart/v2/layout.hh"
#include "utils/log.hh"
#include "chart/v2/lab.hh"
#include "chart/v2/iterator.hh"
#include "utils/regex.hh"
#include "utils/named-vector.hh"
#include "sequences/lineage.hh"
#include "virus/type-subtype.hh"
#include "virus/name.hh"
#include "virus/name-parse.hh"
#include "virus/passage.hh"
#include "virus/reassortant.hh"
#include "locdb/v3/locdb.hh"
#include "chart/v2/name-format.hh"
#include "chart/v2/annotations.hh"
#include "chart/v2/titers.hh"
#include "chart/v2/stress.hh"
#include "chart/v2/optimize.hh"
#include "chart/v2/blobs.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v2
{
    class invalid_data : public std::runtime_error
    {
      public:
        template <typename S> invalid_data(S&& msg) : std::runtime_error{fmt::format("invalid_data: {}", std::forward<S>(msg))} {}
    };
    class chart_is_read_only : public std::runtime_error
    {
      public:
        template <typename S> chart_is_read_only(S&& msg) : std::runtime_error{fmt::format("chart_is_read_only: ", std::forward<S>(msg))} {}
    };

    enum class find_homologous {
        strict,         // passage must match
        relaxed_strict, // if serum has no homologous antigen, relax passage matching for it but use only not previous matched with other sera antigens
        relaxed,        // if serum has no homologous antigen, relax passage matching for it and use any antigen
        all             // find all possible antigens with relaxed passage matching, use for serum circles
    };

    enum class reassortant_as_egg { no, yes };

    class Chart;

    // ----------------------------------------------------------------------

    class Virus : public ae::named_string_t<std::string, struct chart_virus_tag_t>
    {
      public:
        using ae::named_string_t<std::string, struct chart_virus_tag_t>::named_string_t;
    };

    class Assay : public ae::named_string_t<std::string, struct chart_assay_tag_t>
    {
      public:
        using ae::named_string_t<std::string, struct chart_assay_tag_t>::named_string_t;
        enum class no_hi { no, yes };

        std::string hi_or_neut(no_hi nh = no_hi::no) const
        {
            if (empty() || get() == "HI") {
                if (nh == no_hi::no)
                    return "hi";
                else
                    return "";
            }
            else if (get() == "HINT")
                return "hint";
            else
                return "neut";
        }

        std::string HI_or_Neut(no_hi nh = no_hi::no) const
        {
            if (empty() || get() == "HI") {
                if (nh == no_hi::no)
                    return "HI";
                else
                    return "";
            }
            else if (get() == "HINT")
                return "HINT";
            else
                return "Neut";
        }

        std::string short_name() const
        {
            if (get() == "FOCUS REDUCTION")
                return "FRA";
            else
                return get();
        }
    };

    class RbcSpecies : public ae::named_string_t<std::string, struct chart_rbc_species_tag_t>
    {
      public:
        using ae::named_string_t<std::string, struct chart_rbc_species_tag_t>::named_string_t;
    };

    inline std::string assay_rbc_short(const Assay& assay, const RbcSpecies& rbc)
    {
        if (assay.empty() || assay.get() == "HI" || !rbc.empty())
            return fmt::format("{} {}", assay.short_name(), rbc);
        else
            return assay.short_name();
    }

    class TableDate : public ae::named_string_t<std::string, struct chart_table_date_tag_t>
    {
      public:
        using ae::named_string_t<std::string, struct chart_table_date_tag_t>::named_string_t;
    };

    class Date : public ae::named_string_t<std::string, struct chart_date_tag_t>
    {
      public:
        using ae::named_string_t<std::string, struct chart_date_tag_t>::named_string_t;

        bool within_range(std::string_view first_date, std::string_view after_last_date) const
        {
            return !empty() && (first_date.empty() || *this >= first_date) && (after_last_date.empty() || *this < after_last_date);
        }

        // void check() const;

    }; // class Date

    class BLineage
    {
      public:
        enum Lineage { Unknown, Victoria, Yamagata };

        BLineage() = default;
        BLineage(Lineage lineage) : mLineage{lineage} {}
        BLineage(const BLineage&) = default;
        BLineage(std::string_view lineage) : mLineage{from(lineage)} {}
        // BLineage(const std::string& lineage) : BLineage{std::string_view{lineage}} {}
        BLineage(char lineage) : mLineage{from(lineage)} {}
        BLineage& operator=(Lineage lineage)
        {
            mLineage = lineage;
            return *this;
        }
        BLineage& operator=(const BLineage&) = default;
        BLineage& operator=(std::string_view lineage)
        {
            mLineage = from(lineage);
            return *this;
        }
        bool operator==(BLineage lineage) const { return mLineage == lineage.mLineage; }
        bool operator==(Lineage lineage) const { return mLineage == lineage; }

        bool operator==(std::string_view rhs) const
        {
            if (rhs.empty())
                return mLineage == Unknown;
            switch (rhs.front()) {
              case 'V':
              case 'v':
                  return mLineage == Victoria;
              case 'Y':
              case 'y':
                  return mLineage == Yamagata;
            }
            return mLineage == Unknown;
        }

        std::string to_string() const
        {
            using namespace std::string_literals;
            switch (mLineage) {
                case Victoria:
                    return "VICTORIA"s;
                case Yamagata:
                    return "YAMAGATA"s;
                case Unknown:
                    return {};
            }
#ifndef __clang__
            return "UNKNOWN"s;
#endif
        }

        operator ae::sequences::lineage_t() const
        {
            return ae::sequences::lineage_t{to_string()};
        }

        operator Lineage() const { return mLineage; }

      private:
        Lineage mLineage{Unknown};

        static Lineage from(std::string_view aSource);
        static Lineage from(char aSource);

    }; // class BLineage

    class Continent : public ae::named_string_t<std::string, struct chart_continent_tag_t>
    {
      public:
        using ae::named_string_t<std::string, struct chart_continent_tag_t>::named_string_t;

    }; // class Continent

    class LabIds : public ae::named_vector_t<std::string, struct chart_LabIds_tag_t>
    {
      public:
        using ae::named_vector_t<std::string, struct chart_LabIds_tag_t>::named_vector_t;

        LabIds(const rjson::value& src) : ae::named_vector_t<std::string, struct chart_LabIds_tag_t>::named_vector_t(src.size()) { rjson::copy(src, begin()); }

        std::string join() const { return fmt::format("{}", fmt::join(*this, " ")); }

    }; // class LabIds

    class Clades : public ae::named_vector_t<std::string, struct chart_Clades_tag_t>
    {
      public:
        using ae::named_vector_t<std::string, struct chart_Clades_tag_t>::named_vector_t;
        Clades(const rjson::value& src) : ae::named_vector_t<std::string, struct chart_Clades_tag_t>::named_vector_t(src.size()) { rjson::copy(src, begin()); }

    }; // class Clades

    class SerumId : public ae::named_string_t<std::string, struct chart_SerumId_tag_t>
    {
      public:
        using ae::named_string_t<std::string, struct chart_SerumId_tag_t>::named_string_t;

    }; // class SerumId

    class SerumSpecies : public ae::named_string_t<std::string, struct chart_SerumSpecies_tag_t>
    {
      public:
        using ae::named_string_t<std::string, struct chart_SerumSpecies_tag_t>::named_string_t;

    }; // class SerumSpecies

    class DrawingOrder : public ae::named_vector_t<size_t, struct chart_DrawingOrder_tag_t>
    {
      public:
        using ae::named_vector_t<size_t, struct chart_DrawingOrder_tag_t>::named_vector_t;
        DrawingOrder(const rjson::value& src) : ae::named_vector_t<size_t, struct chart_DrawingOrder_tag_t>::named_vector_t(src.size()) { rjson::copy(src, begin()); }

        size_t index_of(size_t aValue) const { return static_cast<size_t>(std::find(begin(), end(), aValue) - begin()); }

        void raise(size_t aIndex)
        {
            if (const auto p = std::find(begin(), end(), aIndex); p != end())
                std::rotate(p, p + 1, end());
        }

        void raise(const std::vector<size_t>& aIndexes)
        {
            std::for_each(aIndexes.begin(), aIndexes.end(), [this](size_t index) { this->raise(index); });
        }

        void lower(size_t aIndex)
        {
            if (const auto p = std::find(rbegin(), rend(), aIndex); p != rend())
                std::rotate(p, p + 1, rend());
        }

        void lower(const std::vector<size_t>& aIndexes)
        {
            std::for_each(aIndexes.begin(), aIndexes.end(), [this](size_t index) { this->lower(index); });
        }

        void fill_if_empty(size_t aSize)
        {
            if (get().empty()) {
                get().resize(aSize);
                std::iota(begin(), end(), 0);
            }
        }

        void insert(size_t before)
        {
            std::for_each(begin(), end(), [before](size_t& point_no) {
                if (point_no >= before)
                    ++point_no;
            });
            push_back(before);
        }

        void remove_points(const ReverseSortedIndexes& to_remove, size_t base_index = 0)
        {
            for (const auto index : to_remove) {
                const auto real_index = index + base_index;
                remove(real_index);
                // if (const auto found = std::find(begin(), end(), real_index); found != end())
                //     erase(found);
                std::for_each(begin(), end(), [real_index](size_t& point_no) {
                    if (point_no > real_index)
                        --point_no;
                });
            }
        }

    }; // class DrawingOrder

    // ----------------------------------------------------------------------

    class Info
    {
      public:
        enum class Compute { No, Yes };
        enum class FixLab { no, yes, reverse };

        virtual ~Info() = default;
        Info() = default;
        Info(const Info&) = delete;

        virtual std::string make_info() const;
        std::string make_name() const;

        virtual std::string name(Compute = Compute::No) const = 0;
        std::string name_non_empty() const
        {
            const auto result = name(Compute::Yes);
            return result.empty() ? std::string{"UNKNOWN"} : result;
        }
        virtual Virus virus(Compute = Compute::No) const = 0;
        Virus virus_not_influenza(Compute aCompute = Compute::No) const
        {
            const auto vir = virus(aCompute);
            if (string::lowercase(*vir) == "influenza")
              return {};
            else
              return vir;
        }
        virtual ae::virus::type_subtype_t virus_type(Compute = Compute::Yes) const = 0;
        virtual std::string subset(Compute = Compute::No) const = 0;
        virtual Assay assay(Compute = Compute::Yes) const = 0;
        virtual Lab lab(Compute = Compute::Yes, FixLab fix = FixLab::yes) const = 0;
        virtual RbcSpecies rbc_species(Compute = Compute::Yes) const = 0;
        virtual TableDate date(Compute aCompute = Compute::No) const = 0;
        virtual size_t number_of_sources() const = 0;
        virtual std::shared_ptr<Info> source(size_t aSourceNo) const = 0;
        size_t max_source_name() const;

      protected:
        Lab fix_lab_name(Lab source, FixLab fix) const;

    }; // class Info

    // ----------------------------------------------------------------------

    namespace detail
    {
        struct location_data_t
        {
            std::string name{};
            std::string country{};
            std::string continent{};
            ae::locdb::v3::Latitude latitude{0.0};
            ae::locdb::v3::Longitude longitude{0.0};
        };

        class AntigenSerum
        {
          public:
            virtual ~AntigenSerum() = default;
            AntigenSerum() = default;
            AntigenSerum(const AntigenSerum&) = delete;
            bool operator==(const AntigenSerum& rhs) const { return name_full() == rhs.name_full(); }

            virtual ae::virus::Name name() const = 0;
            virtual ae::virus::Passage passage() const = 0;
            virtual BLineage lineage() const = 0;
            virtual ae::virus::Reassortant reassortant() const = 0;
            virtual Annotations annotations() const = 0;
            virtual Clades clades() const = 0;
            virtual bool sequenced() const { return false; }
            virtual std::string sequence_aa() const { return {}; }
            virtual std::string sequence_nuc() const { return {}; }

            virtual Continent continent() const { return {}; }

            virtual void format(fmt::memory_buffer& output, std::string_view pattern) const = 0;
            std::string format(std::string_view pattern, collapse_spaces_t cs = collapse_spaces_t::no) const;
            std::string name_full() const { return format("{name_full}"); }

            virtual bool is_egg(reassortant_as_egg rae = reassortant_as_egg::yes) const
            {
                return rae == reassortant_as_egg::yes ? (!reassortant().empty() || passage().is_egg()) : (reassortant().empty() && passage().is_egg());
            }
            bool is_cell() const { return !reassortant().empty() || passage().is_cell(); }

            std::string_view passage_type(reassortant_as_egg rae = reassortant_as_egg::yes) const
            {
                using namespace std::string_view_literals;
                if (is_egg(reassortant_as_egg::yes)) {
                    if (reassortant().empty() || rae == reassortant_as_egg::yes)
                        return "egg"sv;
                    else
                        return "reassortant"sv;
                }
                else
                    return "cell"sv;
            }

            bool distinct() const { return annotations().distinct(); }

            const location_data_t& location_data() const; // name-format.cc

          private:
            mutable bool location_data_filled_{false};
            mutable location_data_t location_data_{};
        };

    } // namespace detail

    // ----------------------------------------------------------------------

    class Antigen : public detail::AntigenSerum
    {
      public:

        virtual Date date() const = 0;
        virtual LabIds lab_ids() const = 0;
        virtual bool reference() const = 0;

        using detail::AntigenSerum::format;
            // returns if collapsable spaces inserted
        void format(fmt::memory_buffer& output, std::string_view pattern) const override; // name-format.cc

    }; // class Antigen

    // ----------------------------------------------------------------------

    class Serum : public detail::AntigenSerum
    {
      public:
        virtual SerumId serum_id() const = 0;
        virtual SerumSpecies serum_species() const = 0;
        virtual PointIndexList homologous_antigens() const = 0;
        virtual void set_homologous(const std::vector<size_t>&, ae::debug) const {}

        using detail::AntigenSerum::format;
            // returns if collapsable spaces inserted
        void format(fmt::memory_buffer& output, std::string_view pattern) const override; // name-format.cc

        bool is_egg(reassortant_as_egg rae = reassortant_as_egg::yes) const override
        {
            const auto egg = passage().is_egg() || serum_id().find("EGG") != std::string::npos;
            return rae == reassortant_as_egg::yes ? (!reassortant().empty() || egg) : (reassortant().empty() && egg);
        }

    }; // class Serum

    // ----------------------------------------------------------------------

    // Argument for antigen/serum selecting object
    template <typename AgSr> struct SelectionData
    {
        size_t index{0};
        size_t point_no{0};
        std::shared_ptr<AgSr> ag_sr{};
        PointCoordinates coord{};
        std::shared_ptr<Titers> titers{nullptr};
    };

    // ----------------------------------------------------------------------

    using duplicates_t = std::vector<std::vector<size_t>>;

    namespace detail
    {
        template <typename AgSr> class AntigensSera
        {
          public:
            using AntigenSerumType = AgSr;

            virtual ~AntigensSera() = default;
            AntigensSera() = default;
            AntigensSera(const AntigensSera&) = delete;

            virtual size_t size() const = 0;
            virtual std::shared_ptr<AgSr> operator[](size_t aIndex) const = 0;
            std::shared_ptr<AgSr> at(size_t aIndex) const { return operator[](aIndex); }

            using iterator = acmacs::iterator<AntigensSera<AgSr>, std::shared_ptr<AgSr>>;
            iterator begin() const { return {*this, 0}; }
            iterator end() const { return {*this, size()}; }

            Indexes all_indexes() const { return filled_with_indexes(size()); }

            // call func for each antigen and select ag/sr if func returns true
            template <typename F> Indexes indexes(F&& func, std::shared_ptr<Titers> titers) const { return make_indexes(std::forward<F>(func), titers); }
            template <typename F> Indexes indexes(const Layout& layout, size_t index_base, F&& func, std::shared_ptr<Titers> titers) const { return make_indexes(layout, index_base, std::forward<F>(func), titers); }

            void filter_found_in(Indexes& aIndexes, const AntigensSera<AgSr>& aNother) const
            {
                remove(aIndexes, [&](const auto& entry) -> bool { return !aNother.find_by_full_name(entry.name_full()); });
            }
            void filter_not_found_in(Indexes& aIndexes, const AntigensSera<AgSr>& aNother) const
            {
                remove(aIndexes, [&](const auto& entry) -> bool { return aNother.find_by_full_name(entry.name_full()).has_value(); });
            }

            Indexes reassortant_indexes() const
            {
                return make_indexes([](const AgSr& ag) { return !ag.reassortant().empty(); });
            }
            void filter_reassortant(Indexes& aIndexes) const
            {
                remove(aIndexes, [](const auto& entry) -> bool { return entry.reassortant().empty(); });
            }

            void filter_egg(Indexes& aIndexes, reassortant_as_egg rae = reassortant_as_egg::yes) const
            {
                remove(aIndexes, [rae](const auto& entry) -> bool { return !entry.is_egg(rae); });
            }
            void filter_cell(Indexes& aIndexes) const
            {
                remove(aIndexes, [](const auto& entry) -> bool { return !entry.is_cell(); });
            }
            void filter_passage(Indexes& aIndexes, std::string_view passage) const
            {
                remove(aIndexes, [passage](const auto& entry) -> bool { return entry.passage().find(passage) == std::string::npos; });
            }
            void filter_passage(Indexes& aIndexes, const std::regex& passage) const
            {
                remove(aIndexes, [&passage](const auto& entry) -> bool { return !entry.passage().search(passage); });
            }

            void filter_lineage(Indexes& aIndexes, BLineage lineage) const
            {
                remove(aIndexes, [lineage](const auto& entry) -> bool { return entry.lineage() != lineage; });
            }

            void filter_country(Indexes& aIndexes, std::string_view aCountry) const
            {
                remove(aIndexes, [aCountry](const auto& entry) {
                    try {
                        return ae::locdb::get().country(ae::virus::name::location(entry.name())) != aCountry;
                    }
                    catch (std::exception&) {
                        return true;
                    }
                });
            }

            void filter_continent(Indexes& aIndexes, std::string_view aContinent) const
            {
                remove(aIndexes, [aContinent](const auto& entry) {
                    try {
                        return ae::locdb::get().continent(ae::virus::name::location(entry.name())) != aContinent;
                    }
                    catch (std::exception&) {
                        return true;
                    }
                });
            }

            void filter_out_distinct(Indexes& aIndexes) const
            {
                remove(aIndexes, [&](const auto& entry) -> bool { return entry.annotations().distinct(); });
            }

            template <typename F> void remove(Indexes& aIndexes, F&& aFilter) const
            {
                aIndexes.erase(std::remove_if(aIndexes.begin(), aIndexes.end(), [&aFilter, this](auto index) -> bool { return aFilter(*(*this)[index]); }), aIndexes.end());
            }

            // if aName starts with ~, then search by regex in full name
            virtual bool name_matches(size_t index, std::string_view aName) const
            {
                if (!aName.empty() && aName[0] == '~') {
                    const std::regex re{std::next(std::begin(aName), 1), std::end(aName), ae::regex::icase};
                    return std::regex_search(at(index)->name_full(), re);
                }
                else {
                    const auto name = at(index)->name();
                    if (name == ae::virus::Name{aName})
                        return true;
                    else if (aName.size() > 2 && name.size() > 2) {
                        if ((aName[0] == 'A' && aName[1] == '/' && name[0] == 'A' && name[1] == '(' && name.find(")/") != std::string::npos) || (aName[0] == 'B' && aName[1] == '/'))
                            return name == ae::virus::Name{fmt::format("{}{}", name.substr(0, name.find('/')), aName.substr(1))};
                        else if (aName[1] != '/' && aName[1] != '(')
                            return name == ae::virus::Name{fmt::format("{}{}", name.substr(0, name.find('/') + 1), aName)};
                    }
                    else
                        return false;
                }
                return false; // bug in clang-10?
            }

            virtual std::optional<size_t> find_by_full_name(std::string_view aFullName) const
            {
                if (const auto found = std::find_if(begin(), end(), [aFullName](const auto& antigen) -> bool { return antigen->name_full() == aFullName; }); found == end())
                    return std::nullopt;
                else
                    return found.index();
            }

            // if aName starts with ~, then search by regex in full name
            virtual SortedIndexes find_by_name(std::string_view aName) const
            {
                if (!aName.empty() && aName[0] == '~')
                    return find_by_name(std::regex{std::next(std::begin(aName), 1), std::end(aName), ae::regex::icase});

                auto find = [this](auto name) {
                    Indexes indexes;
                    for (auto iter = begin(); iter != end(); ++iter) {
                        if ((*iter)->name() == ae::virus::Name{name})
                            indexes.push_back(iter.index());
                    }
                    return indexes;
                };

                const auto name{string::uppercase(aName)};
                auto indexes = find(name);
                if (indexes.empty() && name.size() > 2) {
                    if (const auto first_name = (*begin())->name(); first_name.size() > 2) {
                        // handle names with "A/" instead of "A(HxNx)/" or without subtype prefix (for A and B)
                        if ((name[0] == 'A' && name[1] == '/' && first_name[0] == 'A' && first_name[1] == '(' && first_name.find(")/") != std::string::npos) || (name[0] == 'B' && name[1] == '/'))
                            indexes = find(fmt::format("{}{}", first_name.substr(0, first_name.find('/')), name.substr(1)));
                        else if (name[1] != '/' && name[1] != '(')
                            indexes = find(fmt::format("{}{}", first_name.substr(0, first_name.find('/') + 1), name));
                    }
                }
                return SortedIndexes{indexes};
            }

            // regex search in full name
            virtual SortedIndexes find_by_name(const std::regex& re_name) const
            {
                Indexes indexes;
                for (auto iter = begin(); iter != end(); ++iter) {
                    if (std::regex_search((*iter)->name_full(), re_name))
                        indexes.push_back(iter.index());
                }
                return SortedIndexes{indexes};
            }

            duplicates_t find_duplicates() const
            {
                std::map<std::string, std::vector<size_t>> designations_to_indexes;
                for (size_t index = 0; index < size(); ++index) {
                    auto [pos, inserted] = designations_to_indexes.insert({at(index)->format("{designation}"), {}});
                    pos->second.push_back(index);
                }

                ae::chart::v2::duplicates_t result;
                for (auto [designation, indexes] : designations_to_indexes) {
                    if (indexes.size() > 1 && designation.find(" DISTINCT") == std::string::npos) {
                        result.push_back(indexes);
                    }
                }
                return result;
            }

          protected:
            template <typename F> SortedIndexes make_indexes(F&& test, std::shared_ptr<Titers> titers = nullptr) const
            {
                const auto call = [&](size_t no, const std::shared_ptr<AgSr>& ptr) -> bool {
                    if constexpr (std::is_invocable_v<F, const AgSr&>)
                        return test(*ptr);
                    else if constexpr (std::is_invocable_v<F, size_t, const AgSr&>)
                        return test(no, *ptr);
                    else if constexpr (std::is_invocable_v<F, std::shared_ptr<AgSr>>)
                        return test(ptr);
                    else if constexpr (std::is_invocable_v<F, size_t, std::shared_ptr<AgSr>>)
                        return test(no, ptr);
                    else if constexpr (std::is_invocable_v<F, const SelectionData<AgSr>&>)
                        return test(
                            SelectionData<AntigenSerumType>{.index = no, .point_no = static_cast<size_t>(-1), .ag_sr = ptr, .coord = PointCoordinates{PointCoordinates::nan2D}, .titers = titers});
                    else
                        static_assert(std::is_invocable_v<F, void, int>, "unsupported filter function signature");
                };

                Indexes result;
                for (size_t no = 0; no < size(); ++no) {
                    if (call(no, at(no)))
                        result.push_back(no);
                }
                return SortedIndexes{result};
            }

            template <typename F> SortedIndexes make_indexes(const Layout& layout, size_t index_base, F&& test, std::shared_ptr<Titers> titers) const
            {
                Indexes result;
                for (size_t no = 0; no < size(); ++no) {
                    if (test(SelectionData<AntigenSerumType>{.index = no, .point_no = no + index_base, .ag_sr = at(no), .coord = layout.at(no + index_base), .titers = titers}))
                        result.push_back(no);
                }
                return SortedIndexes{result};
            }
        };

    } // namespace detail

    // ----------------------------------------------------------------------

    class Antigens : public detail::AntigensSera<Antigen>
    {
      public:
        using detail::AntigensSera<Antigen>::AntigensSera;

        SortedIndexes reference_indexes() const
        {
            return make_indexes([](const Antigen& ag) { return ag.reference(); });
        }
        SortedIndexes test_indexes() const
        {
            return make_indexes([](const Antigen& ag) { return !ag.reference(); });
        }
        SortedIndexes egg_indexes() const
        {
            return make_indexes([](const Antigen& ag) { return ag.passage().is_egg() || !ag.reassortant().empty(); });
        }

        void filter_reference(Indexes& aIndexes) const
        {
            remove(aIndexes, [](const auto& entry) -> bool { return !entry.reference(); });
        }
        void filter_test(Indexes& aIndexes) const
        {
            remove(aIndexes, [](const auto& entry) -> bool { return entry.reference(); });
        }

        void filter_date_range(Indexes& aIndexes, std::string_view first_date, std::string_view after_last_date) const
        {
            remove(aIndexes, [first_date, after_last_date](const auto& entry) { return !entry.date().within_range(first_date, after_last_date); });
        }
        void filter_date_not_in_range(Indexes& aIndexes, std::string_view first_date, std::string_view after_last_date) const
        {
            remove(aIndexes, [first_date, after_last_date](const auto& entry) { return entry.date().within_range(first_date, after_last_date); });
        }

        enum class include_reference { no, yes };
        std::vector<Date> all_dates(include_reference inc_ref) const; // list of unique dates of the antigens of a chart

    }; // class Antigens

    // ----------------------------------------------------------------------

    class Sera : public detail::AntigensSera<Serum>
    {
      public:
        using detail::AntigensSera<Serum>::AntigensSera;

        void filter_serum_id(Indexes& aIndexes, std::string_view aSerumId) const
        {
            remove(aIndexes, [&aSerumId](const auto& entry) -> bool { return entry.serum_id() != SerumId{aSerumId}; });
        }

        void filter_serum_id(Indexes& aIndexes, const std::regex& re_serum_id) const
        {
            remove(aIndexes, [&re_serum_id](const auto& entry) -> bool { return !std::regex_search(*entry.serum_id(), re_serum_id); });
        }

        void set_homologous(find_homologous options, const Antigens& aAntigens, ae::debug dbg = ae::debug::no);

      private:
        using homologous_canditate_t = Indexes;                              // indexes of antigens
        using homologous_canditates_t = std::vector<homologous_canditate_t>; // for each serum
        homologous_canditates_t find_homologous_canditates(const Antigens& aAntigens, ae::debug dbg) const;

    }; // class Sera

    // ----------------------------------------------------------------------

    enum class RecalculateStress { no, if_necessary, yes };
    constexpr const double InvalidStress{-1.0};

    class Projection
    {
      public:
        virtual ~Projection() = default;
        Projection(const Chart& chart) : chart_(chart) {}
        Projection(const Projection&) = delete;

        virtual size_t projection_no() const
        {
            if (!projection_no_)
                throw invalid_data("no projection_no");
            return *projection_no_;
        }
        virtual std::string make_info() const;
        virtual std::optional<double> stored_stress() const = 0;
        double stress(RecalculateStress recalculate = RecalculateStress::if_necessary) const;
        double stress_with_moved_point(size_t point_no, const PointCoordinates& move_to) const;
        virtual std::string comment() const = 0;
        virtual number_of_dimensions_t number_of_dimensions() const = 0;
        virtual size_t number_of_points() const = 0;
        virtual std::shared_ptr<Layout> layout() const = 0;
        virtual std::shared_ptr<Layout> transformed_layout() const { return layout()->transform(transformation()); }
        virtual MinimumColumnBasis minimum_column_basis() const = 0;
        virtual std::shared_ptr<ColumnBases> forced_column_bases() const = 0; // returns nullptr if not forced
        virtual draw::v1::Transformation transformation() const = 0;
        virtual enum dodgy_titer_is_regular dodgy_titer_is_regular() const = 0;
        virtual double stress_diff_to_stop() const = 0;
        virtual UnmovablePoints unmovable() const = 0;
        virtual DisconnectedPoints disconnected() const = 0;
        virtual UnmovableInTheLastDimensionPoints unmovable_in_the_last_dimension() const = 0;
        virtual AvidityAdjusts avidity_adjusts() const = 0; // antigens_sera_titers_multipliers, double for each point
                                                            // antigens_sera_gradient_multipliers, double for each point

        double calculate_stress(const Stress& stress) const { return stress.value(*layout()); }
        std::vector<double> calculate_gradient(const Stress& stress) const { return stress.gradient(*layout()); }

        double calculate_stress(multiply_antigen_titer_until_column_adjust mult = multiply_antigen_titer_until_column_adjust::yes) const;
        std::vector<double> calculate_gradient(multiply_antigen_titer_until_column_adjust mult = multiply_antigen_titer_until_column_adjust::yes) const;

        Blobs blobs(double stress_diff, size_t number_of_drections = 36, double stress_diff_precision = 1e-5) const;
        Blobs blobs(double stress_diff, const PointIndexList& points, size_t number_of_drections = 36, double stress_diff_precision = 1e-5) const;

        Chart& chart() { return const_cast<Chart&>(chart_); }
        const Chart& chart() const { return chart_; }
        void set_projection_no(size_t projection_no) { projection_no_ = projection_no; }

        ErrorLines error_lines() const { return ae::chart::v2::error_lines(*this); }

      protected:
        virtual double recalculate_stress() const { return calculate_stress(); }

      private:
        const Chart& chart_;
        std::optional<size_t> projection_no_{std::nullopt};

    }; // class Projection

    // ----------------------------------------------------------------------

    class Projections
    {
      public:
        virtual ~Projections() = default;
        Projections(const Chart& chart) : chart_(chart) {}
        Projections(const Projections&) = delete;

        virtual bool empty() const = 0;
        virtual size_t size() const = 0;
        virtual std::shared_ptr<Projection> operator[](size_t aIndex) const = 0;
        std::shared_ptr<Projection> at(size_t aIndex) const { return operator[](aIndex); }
        virtual std::shared_ptr<Projection> best() const { return operator[](0); }
        using iterator = acmacs::iterator<Projections, std::shared_ptr<Projection>>;
        iterator begin() const { return {*this, 0}; }
        iterator end() const { return {*this, size()}; }
        // virtual size_t projection_no(const Projection* projection) const;

        virtual std::string make_info(size_t max_number_of_projections_to_show = 20) const;

        // Chart& chart() { return const_cast<Chart&>(chart_); }
        const Chart& chart() const { return chart_; }

      private:
        const Chart& chart_;

    }; // class Projections

    // ----------------------------------------------------------------------

    class PlotSpec : public acmacs::PointStyles
    {
      public:
        virtual DrawingOrder drawing_order() const = 0;
        virtual Color error_line_positive_color() const = 0;
        virtual Color error_line_negative_color() const = 0;
        virtual std::vector<acmacs::PointStyle> all_styles() const = 0;
        acmacs::PointStylesCompacted compacted() const override;

    }; // class PlotSpec

    // ----------------------------------------------------------------------

    namespace info_data
    {
        constexpr const unsigned column_bases = 1, tables = 2, tables_for_sera = 4, dates = 8;
    }

    class Chart
    {
      protected:
        enum class PointType { TestAntigen, ReferenceAntigen, Serum };
        acmacs::PointStyle default_style(PointType aPointType) const;

      public:
        enum class use_cache { no, yes };

        virtual ~Chart() = default;
        Chart() = default;
        Chart(const Chart&) = delete;
        Chart(Chart&&) = default;
        Chart& operator=(const Chart&) = delete;
        Chart& operator=(Chart&&) = default;

        virtual std::shared_ptr<Info> info() const = 0;
        virtual std::shared_ptr<Antigens> antigens() const = 0;
        virtual std::shared_ptr<Sera> sera() const = 0;
        virtual std::shared_ptr<Titers> titers() const = 0;
        virtual std::shared_ptr<ColumnBases> forced_column_bases(MinimumColumnBasis aMinimumColumnBasis) const = 0; // returns nullptr if column bases not forced
        virtual std::shared_ptr<ColumnBases> computed_column_bases(MinimumColumnBasis aMinimumColumnBasis, use_cache a_use_cache = use_cache::no) const;
        double column_basis(size_t serum_no, size_t projection_no = 0) const;
        std::shared_ptr<ColumnBases> column_bases(MinimumColumnBasis aMinimumColumnBasis) const;
        virtual std::shared_ptr<Projections> projections() const = 0;
        std::shared_ptr<Projection> projection(size_t aProjectionNo) const { return (*projections())[aProjectionNo]; }
        virtual std::shared_ptr<PlotSpec> plot_spec() const = 0;
        virtual bool is_merge() const = 0;

        virtual size_t number_of_antigens() const { return antigens()->size(); }
        virtual size_t number_of_sera() const { return sera()->size(); }
        size_t number_of_points() const { return number_of_antigens() + number_of_sera(); }
        virtual size_t number_of_projections() const { return projections()->size(); }

        virtual const rjson::value& extension_field(std::string_view /*field_name*/) const { return rjson::ConstNull; }
        virtual const rjson::value& extension_fields() const { return rjson::ConstNull; }

        std::shared_ptr<Antigen> antigen(size_t aAntigenNo) const { return antigens()->operator[](aAntigenNo); }
        std::shared_ptr<Serum> serum(size_t aSerumNo) const { return sera()->operator[](aSerumNo); }
        ae::sequences::lineage_t lineage() const;

        std::string make_info(size_t max_number_of_projections_to_show = 20, unsigned inf = info_data::column_bases|info_data::tables) const;
        std::string make_name(std::optional<size_t> aProjectionNo = {}) const;
        std::string description() const;

        acmacs::PointStyle default_style(size_t aPointNo) const
        {
            auto ags = antigens();
            return default_style(aPointNo < ags->size() ? ((*ags)[aPointNo]->reference() ? PointType::ReferenceAntigen : PointType::TestAntigen) : PointType::Serum);
        }
        std::vector<acmacs::PointStyle> default_all_styles() const;

        void set_homologous(find_homologous options, std::shared_ptr<Sera> aSera = nullptr, ae::debug dbg = ae::debug::no) const;

        Stress make_stress(const Projection& projection, multiply_antigen_titer_until_column_adjust mult = multiply_antigen_titer_until_column_adjust::yes) const
        {
            return stress_factory(projection, mult);
        }

        Stress make_stress(size_t aProjectionNo) const { return make_stress(*projection(aProjectionNo)); }

        std::string show_table(std::optional<size_t> layer_no = {}) const;

        int number_of_digits_for_antigen_serum_index_formatting() const { return static_cast<int>(std::log10(std::max(number_of_antigens(), number_of_sera()))) + 1; }

        template <typename AgSr, typename F> Indexes indexes(const AgSr& ag_sr, F&& func, size_t projection_no) const
        {
            auto prj = projections();
            if (projection_no < prj->size()) {
                size_t index_base{0};
                if constexpr (std::is_base_of_v<Sera, AgSr>)
                    index_base = number_of_antigens();
                return ag_sr.indexes(*prj->at(projection_no)->transformed_layout(), index_base, std::forward<F>(func), titers());
            }
            else {
                return ag_sr.indexes(std::forward<F>(func), titers());
            }
        }

        // check if at least one antigen has a sequences (sequence_aa or sequence_nuc)
        virtual bool has_sequences() const = 0;

      private:
        mutable std::map<MinimumColumnBasis, std::shared_ptr<ColumnBases>> computed_column_bases_{}; // cache, computing might be slow for big charts

    }; // class Chart

    using ChartP = std::shared_ptr<Chart>;
    using AntigenP = std::shared_ptr<Antigen>;
    using SerumP = std::shared_ptr<Serum>;
    using AntigensP = std::shared_ptr<Antigens>;
    using SeraP = std::shared_ptr<Sera>;
    using InfoP = std::shared_ptr<Info>;
    using TitersP = std::shared_ptr<Titers>;
    using ColumnBasesP = std::shared_ptr<ColumnBases>;
    using ProjectionP = std::shared_ptr<Projection>;
    using ProjectionsP = std::shared_ptr<Projections>;
    using PlotSpecP = std::shared_ptr<PlotSpec>;

    inline double Projection::calculate_stress(multiply_antigen_titer_until_column_adjust mult) const { return calculate_stress(stress_factory(*this, mult)); }

    inline std::vector<double> Projection::calculate_gradient(multiply_antigen_titer_until_column_adjust mult) const
    {
        return calculate_gradient(stress_factory(*this, mult));
    }

    template <typename AgSr, typename = std::enable_if_t<std::is_same_v<AgSr, Antigens> || std::is_same_v<AgSr, Sera>>>
        inline bool equal(const AgSr& a1, const AgSr& a2, bool verbose = false)
    {
        if (a1.size() != a2.size()) {
            if (verbose)
                fmt::print(stderr, "WARNING: number of ag/sr different: {} vs {}\n", a1.size(), a2.size());
            return false;
        }
        for (auto i1 = a1.begin(), i2 = a2.begin(); i1 != a1.end(); ++i1, ++i2) {
            if (**i1 != **i2) {
                if (verbose)
                    fmt::print(stderr, "WARNING: ag/sr different: {} vs {}\n", (*i1)->name_full(), (*i2)->name_full());
                return false;
            }
        }
        return true;
    }

    // returns if sets of antigens, sera are the same in both charts and titers are the same.
    // charts may have different sets of projections and different plot specs
    bool same_tables(const Chart& c1, const Chart& c2, bool verbose = false);

    template <typename AgSr> inline size_t max_full_name(const AgSr& ag_sr)
    {
        return std::accumulate(std::begin(ag_sr), std::end(ag_sr), 0ul, [](size_t max_name, const auto& en) { return std::max(max_name, en->name_full().size()); });
    }

    template <typename AgSr> inline size_t max_full_name(const AgSr& ag_sr, const PointIndexList& indexes)
    {
        return std::accumulate(std::begin(indexes), std::end(indexes), 0ul, [&ag_sr](size_t max_name, size_t index) { return std::max(max_name, ag_sr.at(index)->name_full().size()); });
    }

    // ----------------------------------------------------------------------

    TableDate table_date_from_sources(std::vector<std::string>&& sources);

} // namespace ae::chart::v2

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v2::BLineage> : fmt::formatter<ae::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const ae::chart::v2::BLineage& lineage, FormatCtx& ctx) const { return format_to(ctx.out(), "{}", lineage.to_string()); }
};

template <> struct fmt::formatter<ae::chart::v2::Antigen> : fmt::formatter<ae::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const ae::chart::v2::Antigen& antigen, FormatCtx& ctx) const
    {
        return fmt::format_to(ctx.out(), fmt::runtime(antigen.format("{name_full} [{date}]{ }{lab_ids}{ }{lineage}", ae::chart::v2::collapse_spaces_t::yes)));
    }
};

template <> struct fmt::formatter<ae::chart::v2::Serum> : fmt::formatter<ae::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const ae::chart::v2::Serum& serum, FormatCtx& ctx) const
    {
        return fmt::format_to(ctx.out(), fmt::runtime(serum.format("{name_full}{ }{lineage}{ }{species}", ae::chart::v2::collapse_spaces_t::yes)));
    }
};


// ----------------------------------------------------------------------
