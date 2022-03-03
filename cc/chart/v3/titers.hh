#pragma once

#include <vector>

#include "utils/named-type.hh"
#include "chart/v3/index.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Chart;

    // ----------------------------------------------------------------------

    class invalid_titer : public std::runtime_error
    {
      public:
        invalid_titer(std::string msg) : std::runtime_error("invalid titer: " + msg) {}
        invalid_titer(std::string_view msg) : invalid_titer(std::string(msg)) {}
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
            constexpr auto log_titer = [](std::string_view source) -> double { return std::log2(std::stod(std::string{source}) / 10.0); };

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
            // throw invalid_titer{*this}; // for gcc 7.2
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

    class Titers;

    class TiterIterator
    {
      public:
        struct Data
        {
            Data() = default;
            Data(const Data&) = default;
            constexpr operator const Titer& () const { return titer; }
            auto operator<=>(const Data& rhs) const = default;
            Titer titer{};
            antigen_index antigen{};
            serum_index serum{};
        };

        class TiterGetter
        {
          public:
            virtual ~TiterGetter() = default;

            virtual void first(Data& data) const = 0;
            virtual void last(Data& data) const = 0;
            virtual void next(Data& data) const = 0;
        };

        auto operator<=>(const TiterIterator& rhs) const { return data_ <=> rhs.data_; }
        const Data& operator*() const { return data_; }
        const Data* operator->() const { return &data_; }
        const TiterIterator& operator++() { getter_->next(data_); return *this; }

      private:
        Data data_{};
        std::shared_ptr<TiterGetter> getter_{};
        enum class begin { begin };
        enum class end { end };

        TiterIterator(enum begin, std::shared_ptr<TiterGetter> titer_getter) : getter_{titer_getter} { getter_->first(data_); }
        TiterIterator(enum end, std::shared_ptr<TiterGetter> titer_getter) : getter_{titer_getter} { getter_->last(data_); }

        friend class TiterIteratorMaker;

    }; // class TiterIterator

    class TiterIteratorMaker
    {
      public:
        TiterIteratorMaker(std::shared_ptr<TiterIterator::TiterGetter> titer_getter) : getter_{titer_getter} {}
        TiterIterator begin() const { return TiterIterator(TiterIterator::begin::begin, getter_); }
        TiterIterator end() const { return TiterIterator(TiterIterator::end::end, getter_); }

      private:
        std::shared_ptr<TiterIterator::TiterGetter> getter_;
    };

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

        Titers() = default;
        Titers(const Titers&) = default;
        Titers(Titers&&) = default;
        explicit Titers(antigen_index number_of_antigens, serum_index number_of_sera) : number_of_sera_{number_of_sera}, titers_{dense_t(number_of_antigens.get() * number_of_sera.get())} {}

        Titers& operator=(const Titers&) = default;
        Titers& operator=(Titers&&) = default;

        Titer titer(antigen_index aAntigenNo, serum_index aSerumNo) const;
        Titer titer_of_layer(layer_index aLayerNo, antigen_index aAntigenNo, serum_index aSerumNo) const;
        std::vector<Titer> titers_for_layers(antigen_index aAntigenNo, serum_index aSerumNo, include_dotcare inc = include_dotcare::no) const; // returns list of non-dont-care titers in layers, may throw data_not_available
        std::vector<size_t> layers_with_antigen(antigen_index aAntigenNo) const; // returns list of layer indexes that have non-dont-care titers for the antigen, may throw data_not_available
        std::vector<size_t> layers_with_serum(serum_index aSerumNo) const; // returns list of layer indexes that have non-dont-care titers for the serum, may throw data_not_available
        layer_index number_of_layers() const;
        antigen_index number_of_antigens() const;
        serum_index number_of_sera() const;
        size_t number_of_non_dont_cares() const;
        size_t titrations_for_antigen(antigen_index antigen_no) const;
        size_t titrations_for_serum(serum_index serum_no) const;
        double percent_of_non_dont_cares() const { return static_cast<double>(number_of_non_dont_cares()) / static_cast<double>(number_of_antigens().get() * number_of_sera().get()); }
        bool is_dense() const noexcept;

        // std::shared_ptr<ColumnBasesData> computed_column_bases(MinimumColumnBasis aMinimumColumnBasis) const;

        // TableDistances table_distances(const ColumnBases& column_bases, const StressParameters& parameters);
        // void update(TableDistances& table_distances, const ColumnBases& column_bases, const StressParameters& parameters) const;
        // double max_distance(const ColumnBases& column_bases);

        class TiterGetterExisting : public TiterIterator::TiterGetter
        {
          public:
            using titer_getter_t = std::function<Titer (antigen_index, serum_index)>;

            TiterGetterExisting(titer_getter_t a_getter, antigen_index number_of_antigens, serum_index number_of_sera) : getter_{a_getter}, number_of_antigens_{number_of_antigens}, number_of_sera_{number_of_sera} {}
            void first(TiterIterator::Data& data) const override { set(data, antigen_index{0}, serum_index{0}); if (!valid(data.titer)) next(data); }
            void last(TiterIterator::Data& data) const override { data.antigen = number_of_antigens_; data.serum = serum_index{0}; }
            void next(TiterIterator::Data& data) const override
            {
                while (data.antigen < number_of_antigens_) {
                    set(data, data.antigen, data.serum + serum_index{1});
                    if (data.serum >= number_of_sera_)
                        set(data, data.antigen + antigen_index{1}, serum_index{0});
                    if (valid(data.titer))
                        break;
                }
            }

          protected:
            virtual bool valid(const Titer& titer) const { return !titer.is_dont_care(); }

          private:
            titer_getter_t getter_;
            antigen_index number_of_antigens_;
            serum_index number_of_sera_;

            void set(TiterIterator::Data& data, antigen_index ag, serum_index sr) const
            {
                data.antigen = ag;
                data.serum = sr;
                if (ag < number_of_antigens_ && sr < number_of_sera_)
                    data.titer = getter_(ag, sr);
                else
                    data.titer = Titer{};
            }
        };

        class TiterGetterRegular : public TiterGetterExisting
        {
          public:
            using TiterGetterExisting::TiterGetterExisting;

          protected:
            bool valid(const Titer& titer) const override { return titer.is_regular(); }

        };

        TiterIteratorMaker titers_existing() const { return TiterIteratorMaker(std::make_shared<TiterGetterExisting>([this](antigen_index ag, serum_index sr) { return this->titer(ag, sr); }, number_of_antigens(), number_of_sera())); }
        TiterIteratorMaker titers_regular() const { return TiterIteratorMaker(std::make_shared<TiterGetterRegular>([this](antigen_index ag, serum_index sr) { return this->titer(ag, sr); }, number_of_antigens(), number_of_sera())); }
        TiterIteratorMaker titers_existing_from_layer(layer_index layer_no) const { return TiterIteratorMaker(std::make_shared<TiterGetterExisting>([this,layer_no](antigen_index ag, serum_index sr) { return this->titer_of_layer(layer_no, ag, sr); }, number_of_antigens(), number_of_sera())); }

        std::pair<antigen_indexes, serum_indexes> antigens_sera_of_layer(layer_index aLayerNo) const;
        std::pair<antigen_indexes, serum_indexes> antigens_sera_in_multiple_layers() const;
        bool has_morethan_in_layers() const;

        point_indexes having_titers_with(point_index point_no, bool return_point_no = true) const;
          // returns list of points having less than threshold numeric titers
        point_indexes having_too_few_numeric_titers(size_t threshold = 3) const;

        // std::string print() const;

      private:
        serum_index number_of_sera_{0};
        titers_t titers_{};
        layers_t layers_{};
        bool layer_titer_modified_{false}; // force titer recalculation

        static Titer find_titer_for_serum(const sparse_row_t& aRow, serum_index aSerumNo);
        static Titer titer_in_sparse_t(const sparse_t& aSparse, antigen_index aAntigenNo, serum_index aSerumNo);

        void set_titer(dense_t& titers, antigen_index aAntigenNo, serum_index aSerumNo, const Titer& aTiter) { titers[aAntigenNo.get() * number_of_sera_.get() + aSerumNo.get()] = aTiter; }
        void set_titer(sparse_t& titers, antigen_index aAntigenNo, serum_index aSerumNo, const Titer& aTiter);

        std::unique_ptr<titer_merge_report> set_titers_from_layers(more_than_thresholded mtt);
        std::pair<Titer, titer_merge> titer_from_layers(antigen_index aAntigenNo, serum_index aSerumNo, more_than_thresholded mtt, double standard_deviation_threshold) const;

    }; // class Titers

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v3::Titer> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> constexpr auto format(const ae::chart::v3::Titer& titer, FormatCtx& ctx) const { return fmt::format_to(ctx.out(), "{}", titer.get()); }
};

template <> struct fmt::formatter<ae::chart::v3::TiterIterator::Data> : fmt::formatter<ae::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const ae::chart::v3::TiterIterator::Data& value, FormatCtx& ctx) const
    {
        return format_to(ctx.out(), "ag:{} sr:{} t:{}", value.antigen, value.serum, value.titer);
    }
};

// ----------------------------------------------------------------------
