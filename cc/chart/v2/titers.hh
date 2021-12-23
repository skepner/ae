#pragma once

#include <memory>
#include <cmath>
#include <set>

#include "ext/fmt.hh"
#include "ad/rjson-forward.hh"
#include "utils/named-type.hh"
#include "chart/v2/column-bases.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v2
{
    class data_not_available : public std::runtime_error { public: data_not_available(std::string msg) : std::runtime_error("data not available: " + msg) {} };
    class invalid_titer : public std::runtime_error { public: invalid_titer(std::string msg) : std::runtime_error("invalid titer: " + msg) {} invalid_titer(std::string_view msg) : invalid_titer(std::string(msg)) {} };

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
                case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
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

        bool operator<(const Titer& other) const { return value_for_sorting() < other.value_for_sorting(); }
        // bool operator==(const Titer& other) const { return data() == other.data(); }
        // bool operator!=(const Titer& other) const { return ! operator==(other); }

        double logged() const;
        double logged_with_thresholded() const;
        std::string logged_as_string() const;
        double logged_for_column_bases() const;
        size_t value() const;
        size_t value_for_sorting() const;
        size_t value_with_thresholded() const; // returns 20 for <40, 20480 for >10240
        Titer multiplied_by(double value) const; // multiplied_by(2) returns 80 for 40 and <80 for <40, * for *

          // static inline Titer from_logged(double aLogged, std::string aPrefix = "") { return aPrefix + std::to_string(std::lround(std::pow(2.0, aLogged) * 10.0)); }
        static inline Titer from_logged(double aLogged, const char* aPrefix = "") { return Titer{aPrefix + std::to_string(std::lround(std::exp2(aLogged) * 10.0))}; }

        static std::string_view validate(std::string_view titer);

    }; // class Titer

      // inline std::ostream& operator<<(std::ostream& s, const Titer& aTiter) { return s << aTiter; }

    inline double Titer::logged() const
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
                throw invalid_titer(*this);
        }
        throw invalid_titer(*this); // for gcc 7.2

    }

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
            constexpr bool operator==(const Data& rhs) const { return antigen == rhs.antigen && serum == rhs.serum; }
            constexpr bool operator!=(const Data& rhs) const { return !operator==(rhs); }
            Titer titer{};
            size_t antigen{0}, serum{0};
        };

        class TiterGetter
        {
          public:
            virtual ~TiterGetter() = default;

            virtual void first(Data& data) const = 0;
            virtual void last(Data& data) const = 0;
            virtual void next(Data& data) const = 0;
        };

        constexpr bool operator==(const TiterIterator& rhs) const { return data_ == rhs.data_; }
        constexpr bool operator!=(const TiterIterator& rhs) const { return !operator==(rhs); }
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

    inline std::ostream& operator<<(std::ostream& s, const TiterIterator::Data& data) { return s << data.antigen << ':' << data.serum << ':' << *data.titer; }

// ----------------------------------------------------------------------

    class TableDistances;
    class PointIndexList;
    class AvidityAdjusts;
    struct StressParameters;
    class ChartModify;

    class Titers
    {
     public:
        enum class include_dotcare { no, yes };

        static constexpr double dense_sparse_boundary = 0.7;

        virtual ~Titers() {}
        Titers() = default;
        Titers(const Titers&) = delete;

        virtual Titer titer(size_t aAntigenNo, size_t aSerumNo) const = 0;
        virtual Titer titer_of_layer(size_t aLayerNo, size_t aAntigenNo, size_t aSerumNo) const = 0;
        virtual std::vector<Titer> titers_for_layers(size_t aAntigenNo, size_t aSerumNo, include_dotcare inc = include_dotcare::no) const = 0; // returns list of non-dont-care titers in layers, may throw data_not_available
        virtual std::vector<size_t> layers_with_antigen(size_t aAntigenNo) const = 0; // returns list of layer indexes that have non-dont-care titers for the antigen, may throw data_not_available
        virtual std::vector<size_t> layers_with_serum(size_t aSerumNo) const = 0; // returns list of layer indexes that have non-dont-care titers for the serum, may throw data_not_available
        virtual size_t number_of_layers() const = 0;
        virtual size_t number_of_antigens() const = 0;
        virtual size_t number_of_sera() const = 0;
        virtual size_t number_of_non_dont_cares() const = 0;
        virtual size_t titrations_for_antigen(size_t antigen_no) const = 0;
        virtual size_t titrations_for_serum(size_t serum_no) const = 0;
        virtual double percent_of_non_dont_cares() const { return static_cast<double>(number_of_non_dont_cares()) / static_cast<double>(number_of_antigens() * number_of_sera()); }
        virtual bool is_dense() const noexcept;

          // support for fast exporting into ace, if source was ace or acd1
        virtual const rjson::value& rjson_list_list() const { throw data_not_available{"rjson_list_list titers are not available"}; }
        virtual const rjson::value& rjson_list_dict() const { throw data_not_available{"rjson_list_dict titers are not available"}; }
        virtual const rjson::value& rjson_layers() const { throw data_not_available{"rjson_list_dict titers are not available"}; }

        std::shared_ptr<ColumnBasesData> computed_column_bases(MinimumColumnBasis aMinimumColumnBasis) const;

        TableDistances table_distances(const ColumnBases& column_bases, const StressParameters& parameters);
        virtual void update(TableDistances& table_distances, const ColumnBases& column_bases, const StressParameters& parameters) const;
        virtual double max_distance(const ColumnBases& column_bases);

        class TiterGetterExisting : public TiterIterator::TiterGetter
        {
          public:
            using titer_getter_t = std::function<Titer (size_t, size_t)>;

            TiterGetterExisting(titer_getter_t getter, size_t number_of_antigens, size_t number_of_sera) : getter_{getter}, number_of_antigens_{number_of_antigens}, number_of_sera_{number_of_sera} {}
            void first(TiterIterator::Data& data) const override { set(data, 0, 0); if (!valid(data.titer)) next(data); }
            void last(TiterIterator::Data& data) const override { data.antigen = number_of_antigens_; data.serum = 0; }
            void next(TiterIterator::Data& data) const override
            {
                while (data.antigen < number_of_antigens_) {
                    set(data, data.antigen, data.serum + 1);
                    if (data.serum >= number_of_sera_)
                        set(data, data.antigen + 1, 0);
                    if (valid(data.titer))
                        break;
                }
            }

          protected:
            virtual bool valid(const Titer& titer) const { return !titer.is_dont_care(); }

          private:
            titer_getter_t getter_;
            size_t number_of_antigens_, number_of_sera_;

            void set(TiterIterator::Data& data, size_t ag, size_t sr) const
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

        virtual TiterIteratorMaker titers_existing() const { return TiterIteratorMaker(std::make_shared<TiterGetterExisting>([this](size_t ag, size_t sr) { return this->titer(ag, sr); }, number_of_antigens(), number_of_sera())); }
        virtual TiterIteratorMaker titers_regular() const { return TiterIteratorMaker(std::make_shared<TiterGetterRegular>([this](size_t ag, size_t sr) { return this->titer(ag, sr); }, number_of_antigens(), number_of_sera())); }
        virtual TiterIteratorMaker titers_existing_from_layer(size_t layer_no) const { return TiterIteratorMaker(std::make_shared<TiterGetterExisting>([this,layer_no](size_t ag, size_t sr) { return this->titer_of_layer(layer_no, ag, sr); }, number_of_antigens(), number_of_sera())); }

        std::pair<PointIndexList, PointIndexList> antigens_sera_of_layer(size_t aLayerNo) const;
        std::pair<PointIndexList, PointIndexList> antigens_sera_in_multiple_layers() const;
        bool has_morethan_in_layers() const;

        PointIndexList having_titers_with(size_t point_no, bool return_point_no = true) const;
          // returns list of points having less than threshold numeric titers
        PointIndexList having_too_few_numeric_titers(size_t threshold = 3) const;

        std::string print() const;

    }; // class Titers

    bool equal(const Titers& t1, const Titers& t2, bool verbose = false);

} // namespace ae::chart::v2

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v2::Titer> : fmt::formatter<ae::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const ae::chart::v2::Titer& titer, FormatCtx& ctx) const { return fmt::format_to(ctx.out(), "{}", titer.get()); }
};

template <> struct fmt::formatter<ae::chart::v2::TiterIterator::Data> : fmt::formatter<ae::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const ae::chart::v2::TiterIterator::Data& value, FormatCtx& ctx) const
    {
        return format_to(ctx.out(), "ag:{} sr:{} t:{}", value.antigen, value.serum, value.titer);
    }
};

template <> struct fmt::formatter<std::vector<ae::chart::v2::Titer>> : fmt::formatter<ae::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const std::vector<ae::chart::v2::Titer>& titers, FormatCtx& ctx) const
    {
        format_to(ctx.out(), "[");
        bool first{true};
        for (const auto& titer: titers) {
            if (first)
                first = false;
            else
                format_to(ctx.out(), ", ");
            format_to(ctx.out(), "{}", titer);
        }
        return format_to(ctx.out(), "]");
    }
};

template <> struct fmt::formatter<std::set<ae::chart::v2::Titer>> : fmt::formatter<ae::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const std::set<ae::chart::v2::Titer>& titers, FormatCtx& ctx) const
    {
        format_to(ctx.out(), "{{");
        bool first{true};
        for (const auto& titer: titers) {
            if (first)
                first = false;
            else
                format_to(ctx.out(), ", ");
            format_to(ctx.out(), "{}", titer);
        }
        return format_to(ctx.out(), "}}");
    }
};

// ----------------------------------------------------------------------
