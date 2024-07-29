#pragma once

#include <vector>
#include <variant>
#include <memory>

#include "utils/named-type.hh"
#include "chart/v3/index.hh"
#include "chart/v3/column-bases.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Chart;

    // ----------------------------------------------------------------------

    class data_not_available : public std::runtime_error
    {
      public:
        data_not_available(std::string_view msg) : std::runtime_error{fmt::format("data not available: {}", msg)} {}
    };

    class invalid_titer : public std::runtime_error
    {
      public:
        invalid_titer(std::string_view msg) : std::runtime_error{fmt::format("invalid titer: {}", msg)} {}
    };

    // ----------------------------------------------------------------------

    class Titer : public ae::named_string_t<std::string, struct chart_Titer_tag_t>
    {
      public:
        enum Type { Invalid, Regular, DontCare, LessThan, MoreThan, Dodgy };

        using base_t = ae::named_string_t<std::string, struct chart_Titer_tag_t>;

        Titer() : base_t("*") {}
        Titer(char typ, size_t value) : base_t(typ + std::to_string(value)) {}
        Titer(std::string_view source) : base_t(validate(source)) {}
        Titer(const Titer&) = default;
        Titer& operator=(const Titer&) = default;

        Type type() const
        {
            if (empty())
                return Invalid;
            switch (get().front()) {
                case '*':
                    return DontCare;
                case '<':
                    return LessThan;
                case '>':
                    return MoreThan;
                case '~':
                    return Dodgy;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    return Regular;
                default:
                    return Invalid;
            }
        }

        bool is_invalid() const { return type() == Invalid; }
        bool is_dont_care() const { return type() == DontCare; }
        bool is_regular() const { return type() == Regular; }
        bool is_less_than() const { return type() == LessThan; }
        bool is_more_than() const { return type() == MoreThan; }

        auto operator<=>(const Titer& other) const
        {
            if (ae::named_string_t<std::string, struct chart_Titer_tag_t>::operator==(other))
                return std::strong_ordering::equal;
            else
                return value_for_sorting() <=> other.value_for_sorting();
        }

        double logged() const
        {
            const auto log_titer = [](std::string_view source) -> double { return std::log2(std::stod(std::string{source}) / 10.0); };

            switch (type()) {
                case Regular:
                    return log_titer(*this);
                case LessThan:
                case MoreThan:
                case Dodgy:
                    return log_titer(get().substr(1));
                case DontCare:
                case Invalid:
                    throw invalid_titer{*this};
            }
            throw invalid_titer{*this}; // for g++ 11
        }

        double logged_with_thresholded() const;
        std::string logged_as_string() const;
        double logged_for_column_bases() const;
        size_t value() const;
        size_t value_for_sorting() const;
        size_t value_with_thresholded() const;   // returns 20 for <40, 20480 for >10240
        Titer multiplied_by(double value) const; // multiplied_by(2) returns 80 for 40 and <80 for <40, * for *

        // static inline Titer from_logged(double aLogged, std::string aPrefix = "") { return aPrefix + std::to_string(std::lround(std::pow(2.0, aLogged) * 10.0)); }
        static inline Titer from_logged(double aLogged, const char* aPrefix = "") { return Titer{aPrefix + std::to_string(std::lround(std::exp2(aLogged) * 10.0))}; }

        static std::string_view validate(std::string_view titer);

    }; // class Titer

    // ----------------------------------------------------------------------

    class Titers
    {
      public:
        enum class include_dotcare { no, yes };

        static constexpr double dense_sparse_boundary = 0.7;

        using dense_t = std::vector<Titer>;
        using sparse_entry_t = std::pair<serum_index, Titer>;
        using sparse_row_t = std::vector<sparse_entry_t>;
        using sparse_t = std::vector<sparse_row_t>; // size = number_of_antigens
        using titers_t = std::variant<dense_t, sparse_t>;
        using layers_t = std::vector<sparse_t>;

        enum class titer_merge {
            all_dontcare,
            less_and_more_than,
            less_than_only,
            more_than_only_adjust_to_next,
            more_than_only_to_dontcare,
            sd_too_big,
            regular_only,
            max_less_than_bigger_than_max_regular,
            less_than_and_regular,
            min_more_than_less_than_min_regular,
            more_than_and_regular
        };

        struct titer_merge_data
        {
            titer_merge_data(Titer&& a_titer, antigen_index ag_no, serum_index sr_no, titer_merge a_report) : titer{std::move(a_titer)}, antigen{ag_no}, serum{sr_no}, report{a_report} {}
            Titer titer;
            antigen_index antigen;
            serum_index serum;
            titer_merge report;
        };

        using titer_merge_report = std::vector<titer_merge_data>;

        enum class more_than_thresholded { adjust_to_next, to_dont_care };

        // ----------------------------------------------------------------------

        class iterator
        {
          public:
            struct ref
            {
                const antigen_index antigen;
                const serum_index serum;
                const Titer& titer;
            };

          private:
            struct data_dense
            {
                const serum_index number_of_sera;
                dense_t::const_iterator current_titer;
                const dense_t::const_iterator begin_titers;
                const dense_t::const_iterator end_titers;

                data_dense(serum_index ns, dense_t::const_iterator current, dense_t::const_iterator first, dense_t::const_iterator last)
                    : number_of_sera{ns}, current_titer{current}, begin_titers{first}, end_titers{last}
                {
                    skip_dont_care();
                }
                bool operator==(const data_dense&) const = default;

                data_dense& operator++()
                {
                    ++current_titer;
                    skip_dont_care();
                    return *this;
                }

                void skip_dont_care()
                {
                    while (current_titer != end_titers && current_titer->is_dont_care())
                        ++current_titer;
                }

                antigen_index antigen() const { return antigen_index{(current_titer - begin_titers) / *number_of_sera}; }
                serum_index serum() const { return serum_index{(current_titer - begin_titers) % *number_of_sera}; }
                ref operator*() const { return ref{antigen(), serum(), *current_titer}; }
            };

            struct data_sparse
            {
                sparse_t::const_iterator current_row;
                const sparse_t::const_iterator begin_rows;
                const sparse_t::const_iterator end_rows;
                sparse_row_t::const_iterator current_titer;
                sparse_row_t::const_iterator end_titers;

                bool operator==(const data_sparse&) const = default;
                data_sparse& operator++();
                antigen_index antigen() const { return antigen_index{current_row - begin_rows}; }
                serum_index serum() const { return current_titer->first; }
                ref operator*() const { return ref{antigen(), serum(), current_titer->second}; }

                void skip_empty_rows();
            };

          public:
            bool operator==(const iterator&) const = default;

            ref operator*() const
            {
                return std::visit([](auto& dat) -> ref { return *dat; }, data_);
            }

            const Titer* operator->() const { return &operator*().titer; }

            iterator& operator++()
            {
                std::visit([](auto& dat) { ++dat; }, data_);
                return *this;
            }

            antigen_index antigen() const
            {
                return std::visit([](const auto& dat) { return dat.antigen(); }, data_);
            }
            serum_index serum() const
            {
                return std::visit([](const auto& dat) { return dat.serum(); }, data_);
            }

          private:
            enum _scroll_to_end { scroll_to_end };

            iterator(const sparse_t& titers, serum_index)
                : data_{data_sparse{.current_row = titers.begin(), .begin_rows = titers.begin(), .end_rows = titers.end(), .current_titer = titers.front().begin(), .end_titers = titers.front().end()}}
            {
                std::visit(
                    []<typename SD>(SD& dat) {
                        if constexpr (std::is_same_v<SD, data_sparse>)
                            dat.skip_empty_rows();
                    },
                    data_);
            }
            iterator(const sparse_t& titers, serum_index, _scroll_to_end)
                : data_{data_sparse{.current_row = titers.end(), .begin_rows = titers.begin(), .end_rows = titers.end(), .current_titer = titers.back().end(), .end_titers = titers.back().end()}}
            {
            }

            iterator(const dense_t& titers, serum_index number_of_sera) : data_{data_dense{number_of_sera, titers.begin(), titers.begin(), titers.end()}} {}
            iterator(const dense_t& titers, serum_index number_of_sera, _scroll_to_end) : data_{data_dense{number_of_sera, titers.end(), titers.begin(), titers.end()}} {}

            std::variant<data_dense, data_sparse> data_;

            friend class Titers;
        };

        // ----------------------------------------------------------------------

        Titers() = default;
        Titers(const Titers&) = default;
        Titers(Titers&&) = default;
        explicit Titers(antigen_index number_of_antigens, serum_index number_of_sera) : number_of_sera_{number_of_sera}, titers_{dense_t(number_of_antigens.get() * number_of_sera.get())} {}

        Titers& operator=(const Titers&) = default;
        Titers& operator=(Titers&&) = default;
        bool operator==(const Titers&) const = default;

        antigen_index number_of_antigens() const;
        serum_index number_of_sera() const { return number_of_sera_; }

        Titer titer(antigen_index aAntigenNo, serum_index aSerumNo) const;

        void set_titer(antigen_index aAntigenNo, serum_index aSerumNo, const Titer& titer)
        {
            std::visit([this, aAntigenNo, aSerumNo, &titer](auto& titers) { set_titer(titers, aAntigenNo, aSerumNo, titer); }, titers_);
        }

        size_t number_of_non_dont_cares() const;
        size_t titrations_for_antigen(antigen_index antigen_no) const;
        size_t titrations_for_serum(serum_index serum_no) const;
        double percent_of_non_dont_cares() const { return static_cast<double>(number_of_non_dont_cares()) / static_cast<double>(number_of_antigens().get() * number_of_sera().get()); }

        double raw_column_basis(serum_index sr_no) const; // raw value, not adjusted by minimum column basis
        column_bases raw_column_bases() const;            // raw values, not adjusted by minimum column basis
        double max_distance(const column_bases& cb) const;

        std::pair<antigen_indexes, serum_indexes> antigens_sera_of_layer(layer_index aLayerNo) const;
        // std::pair<antigen_indexes, serum_indexes> antigens_sera_in_multiple_layers() const;
        bool has_morethan_in_layers() const;

        point_indexes having_titers_with(point_index point_no, bool return_point_no = true) const;
        // returns list of points having less than threshold numeric titers
        point_indexes having_too_few_numeric_titers(size_t threshold = 3) const;

        // importing
        void number_of_sera(serum_index num) { number_of_sera_ = num; }
        dense_t& create_dense_titers()
        {
            titers_ = dense_t{};
            return std::get<dense_t>(titers_);
        }
        sparse_t& create_sparse_titers()
        {
            titers_ = sparse_t{};
            return std::get<sparse_t>(titers_);
        }

        // ----------------------------------------------------------------------

        layer_index number_of_layers() const { return layer_index{layers_.size()}; }
        layers_t& layers() { return layers_; }
        auto& layer(layer_index layer_no) { return layers_[layer_no.get()]; }
        const auto& layer(layer_index layer_no) const { return layers_[layer_no.get()]; }
        void check_layers(layer_index layer_no = layer_index{0}) const
        {
            if (number_of_layers() <= layer_no)
                throw data_not_available{"invalid layer number or no layers present"};
        }
        Titer titer_of_layer(layer_index aLayerNo, antigen_index aAntigenNo, serum_index aSerumNo) const { return titer_in_sparse_t(layers_[aLayerNo.get()], aAntigenNo, aSerumNo); }
        void set_titer_of_layer(layer_index aLayerNo, antigen_index aAntigenNo, serum_index aSerumNo, const Titer& titer) { set_titer(layers_[aLayerNo.get()], aAntigenNo, aSerumNo, titer); }
        std::vector<Titer> titers_for_layers(antigen_index aAntigenNo, serum_index aSerumNo,
                                             include_dotcare inc = include_dotcare::no) const; // returns list of non-dont-care titers in layers, may throw data_not_available
        layer_indexes layers_with_antigen(antigen_index aAntigenNo) const; // returns list of layer indexes that have non-dont-care titers for the antigen, may throw data_not_available
        layer_indexes layers_with_serum(serum_index aSerumNo) const;       // returns list of layer indexes that have non-dont-care titers for the serum, may throw data_not_available
        template <typename Ind> layer_indexes layers_with(Ind no) const { if constexpr (std::is_same_v<Ind, antigen_index>) return layers_with_antigen(no); else return layers_with_serum(no); }
        void create_layers(layer_index num_layers, antigen_index num_antigens);
        titer_merge_report set_from_layers(Chart& chart);
        titer_merge_report set_from_layers_report(more_than_thresholded mtt = more_than_thresholded::to_dont_care) const;

        // ----------------------------------------------------------------------
        // exporting
        bool is_dense() const { return std::holds_alternative<dense_t>(titers_); }
        const dense_t& dense_titers() const { return std::get<dense_t>(titers_); }
        const sparse_t& sparse_titers() const { return std::get<sparse_t>(titers_); }

        // ----------------------------------------------------------------------

        iterator begin() const
        {
            return std::visit([this](const auto& en) { return iterator{en, number_of_sera_}; }, titers_);
        }
        iterator end() const
        {
            return std::visit([this](const auto& en) { return iterator{en, number_of_sera_, iterator::scroll_to_end}; }, titers_);
        }

        class iterator_gen
        {
          public:
            iterator_gen(iterator first, iterator last) : begin_{first}, end_{last} {}
            iterator begin() const { return begin_; }
            iterator end() const { return end_; }

          private:
            iterator begin_, end_;
        };

        iterator_gen titers_existing() const { return iterator_gen{begin(), end()}; }
        // iterator_gen titers_regular() const { throw std::runtime_error{}; }
        iterator_gen titers_existing_from_layer(layer_index layer_no) const
        {
            return iterator_gen{iterator{layers_[*layer_no], number_of_sera_}, iterator{layers_[*layer_no], number_of_sera_, iterator::scroll_to_end}};
        }

        void remove_antigens(const antigen_indexes& to_remove)
        {
            std::visit([&to_remove, this](auto& dat) { Titers::remove_antigens(dat, to_remove, number_of_sera_); }, titers_);
            for (auto& layer : layers_)
                remove_antigens(layer, to_remove, number_of_sera_);
        }

        void remove_sera(const serum_indexes& to_remove)
        {
            std::visit([&to_remove, this](auto& dat) { Titers::remove_sera(dat, to_remove, number_of_sera_); }, titers_);
            for (auto& layer : layers_)
                remove_sera(layer, to_remove, number_of_sera_);
            number_of_sera_ = number_of_sera_ - to_remove.size();
        }

        // ----------------------------------------------------------------------

      private:
        serum_index number_of_sera_{0};
        titers_t titers_{};
        layers_t layers_{};
        bool layer_titer_modified_{false}; // force titer recalculation

        static Titer find_titer_for_serum(const sparse_row_t& aRow, serum_index aSerumNo);
        static inline Titer titer_in_sparse_t(const sparse_t& aSparse, antigen_index aAntigenNo, serum_index aSerumNo) { return find_titer_for_serum(aSparse[aAntigenNo.get()], aSerumNo); }

        void set_titer(dense_t& titers, antigen_index aAntigenNo, serum_index aSerumNo, const Titer& aTiter) { titers[aAntigenNo.get() * number_of_sera_.get() + aSerumNo.get()] = aTiter; }
        void set_titer(sparse_t& titers, antigen_index aAntigenNo, serum_index aSerumNo, const Titer& aTiter);

        std::pair<Titer, titer_merge> merge_titers(const std::vector<Titer>& titers, more_than_thresholded mtt, double standard_deviation_threshold) const;
        titer_merge_report set_titers_from_layers(more_than_thresholded mtt);
        std::pair<Titer, titer_merge> titer_from_layers(antigen_index aAntigenNo, serum_index aSerumNo, more_than_thresholded mtt, double standard_deviation_threshold) const;

        static void remove_antigens(dense_t& data, const antigen_indexes& to_remove, serum_index number_of_sera);
        static void remove_antigens(sparse_t& data, const antigen_indexes& to_remove, serum_index number_of_sera);
        static void remove_sera(dense_t& data, const serum_indexes& to_remove, serum_index number_of_sera);
        static void remove_sera(sparse_t& data, const serum_indexes& to_remove, serum_index number_of_sera);

    }; // class Titers

} // namespace ae::chart::v3

    // ----------------------------------------------------------------------

    template <>
    struct fmt::formatter<ae::chart::v3::Titer> : fmt::formatter<ae::fmt_helper::default_formatter>
    {
         auto format(const ae::chart::v3::Titer& titer, format_context& ctx) const { return fmt::format_to(ctx.out(), "{}", titer.get()); }
    };

template <> struct fmt::formatter<ae::chart::v3::Titers::iterator::ref> : fmt::formatter<ae::fmt_helper::default_formatter>
{
     auto format(const ae::chart::v3::Titers::iterator::ref& value, format_context& ctx) const
    {
        return fmt::format_to(ctx.out(), "ag:{} sr:{} t:{}", value.antigen, value.serum, value.titer);
    }
};

// ----------------------------------------------------------------------
