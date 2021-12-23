#pragma once

#include <memory>

#include "ext/fmt.hh"
#include "utils/float.hh"
#include "utils/string.hh"
#include "chart/v2/indexes.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v2
{
    class MinimumColumnBasis
    {
     public:
        MinimumColumnBasis(double value = 0) : value_{value} {}
        MinimumColumnBasis(const MinimumColumnBasis&) = default;
        MinimumColumnBasis(std::string value) { from(value); }
        MinimumColumnBasis(std::string_view value) { from(value); }
        MinimumColumnBasis(const char* value) { from(value); }
        MinimumColumnBasis& operator=(double value) { value_ = value; return *this; }
        MinimumColumnBasis& operator=(const MinimumColumnBasis&) = default;
        MinimumColumnBasis& operator=(std::string value) { from(value); return *this; }
        bool operator==(const MinimumColumnBasis& rhs) const { return float_equal(value_, rhs.value_); }
        bool operator!=(const MinimumColumnBasis& rhs) const { return !operator==(rhs); }

        constexpr bool is_none() const { return float_zero(value_); }

        constexpr operator double() const noexcept { return value_; }
        double apply(double column_basis) const noexcept { return std::max(column_basis, value_); }

        enum class use_none { no, yes };
        std::string format(std::string_view format, use_none un) const noexcept;
        operator std::string() const noexcept { return format("{}", use_none::yes); }

      private:
        double value_{};

        void from(std::string_view value);

    }; // class MinimumColumnBasis

// ----------------------------------------------------------------------

    class ColumnBases
    {
      public:
        virtual ~ColumnBases() = default;
        ColumnBases() = default;
        ColumnBases(const ColumnBases&) = default;

        virtual double column_basis(size_t aSerumNo) const = 0;
        virtual size_t size() const = 0;

        virtual std::vector<double> data() const
        {
            std::vector<double> result(size());
            for (size_t i = 0; i < size(); ++i)
                result[i] = column_basis(i);
            return result;
        }

    }; // class ColumnBases

// ----------------------------------------------------------------------

    class ColumnBasesData : public ColumnBases
    {
      public:
        ColumnBasesData(size_t number_of_sera, double aMinimumColumnBasis = 0.0) : data_(number_of_sera, aMinimumColumnBasis) {}
        ColumnBasesData(const ColumnBases& aSource) : data_(aSource.size()) { for (size_t serum_no = 0; serum_no < data_.size(); ++serum_no) data_[serum_no] = aSource.column_basis(serum_no); }
        ColumnBasesData(const std::vector<double>& source) : data_{source} {}

        double column_basis(size_t aSerumNo) const override { return data_.at(aSerumNo); }
        size_t size() const override { return data_.size(); }
        std::vector<double> data() const override { return data_; }
        std::vector<double>& data() { return data_; }

        void set(size_t aSerumNo, double column_basis) { data_.at(aSerumNo) = column_basis; }
        void remove(const ReverseSortedIndexes& indexes, ReverseSortedIndexes::difference_type base_index = 0) { ae::chart::v2::remove(indexes, data_, base_index); }
        void insert(size_t before, double value) { data_.insert(data_.begin() + static_cast<decltype(data_)::difference_type>(before), value); }

     private:
        std::vector<double> data_{};

    }; // class ColumnBases

} // namespace ae::chart::v2

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v2::ColumnBases> : fmt::formatter<ae::fmt_helper::float_formatter> {
    template <typename FormatCtx> auto format(const ae::chart::v2::ColumnBases& cb, FormatCtx& ctx) const {
        format_to(ctx.out(), "[");
        for (size_t sr_no{0}; sr_no < cb.size(); ++sr_no) {
            if (sr_no)
                fmt::format_to(ctx.out(), " ");
            format_val(cb.column_basis(sr_no), ctx);
        }
        return format_to(ctx.out(), "]");
    }
};

template <> struct fmt::formatter<std::shared_ptr<ae::chart::v2::ColumnBases>> : fmt::formatter<ae::chart::v2::ColumnBases> {
    template <typename FormatCtx> auto format(const std::shared_ptr<ae::chart::v2::ColumnBases>& cb, FormatCtx& ctx) const {
        if (cb)
            return fmt::formatter<ae::chart::v2::ColumnBases>::format(*cb, ctx);
        else
            return format_to(ctx.out(), "<none>");
    }
};

template <> struct fmt::formatter<ae::chart::v2::MinimumColumnBasis> : fmt::formatter<std::string> {
    template <typename FormatCtx> auto format(const ae::chart::v2::MinimumColumnBasis& mcb, FormatCtx& ctx) const { return fmt::formatter<std::string>::format(static_cast<std::string>(mcb), ctx); }
};

// ----------------------------------------------------------------------
