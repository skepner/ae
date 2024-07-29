#pragma once

#include <memory>

#include "ext/from_chars.hh"
#include "utils/log.hh"
#include "utils/string.hh"
#include "chart/v3/index.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class minimum_column_basis
    {
      public:
        minimum_column_basis(double value = 0) : value_{value} {}
        minimum_column_basis(const minimum_column_basis&) = default;
        minimum_column_basis(std::string value) { from(value); }
        minimum_column_basis(std::string_view value) { from(value); }
        minimum_column_basis(const char* value) { from(value); }
        minimum_column_basis& operator=(double value)
        {
            value_ = value;
            return *this;
        }
        minimum_column_basis& operator=(const minimum_column_basis&) = default;
        minimum_column_basis& operator=(std::string_view value)
        {
            from(value);
            return *this;
        }
        bool operator==(const minimum_column_basis& rhs) const { return float_equal(value_, rhs.value_); }
        bool operator!=(const minimum_column_basis& rhs) const { return !operator==(rhs); }

        bool is_none() const { return float_zero(value_); }

        operator double() const noexcept { return value_; }
        double apply(double column_basis) const noexcept { return std::max(column_basis, value_); }

        enum class use_none { no, yes };
        std::string format(std::string_view format, use_none un) const noexcept
        {
            if (is_none()) {
                if (un == use_none::yes)
                    return fmt::format(fmt::runtime(format), "none");
                else
                    return {};
            }
            else
                return fmt::format(fmt::runtime(format), std::lround(std::exp2(value_) * 10.0));
        }

        operator std::string() const noexcept { return format("{}", use_none::yes); }

      private:
        double value_{};

        void from(std::string_view value)
        {
            if (value.empty() || value == "none") {
                value_ = 0;
            }
            else if (value.find('.') != std::string::npos) {
                value_ = ae::from_chars<double>(value);
            }
            else {
                value_ = static_cast<double>(ae::from_chars<long>(value));
                if (value_ > 9)
                    value_ = std::log2(value_ / 10.0);
            }
            if (value_ < 0 || value_ > 30)
                throw std::runtime_error{AD_FORMAT("Unrecognized minimum_column_basis value: {}", value)};
        }
    }; // class minimum_column_basis

    // ----------------------------------------------------------------------

    class column_bases
    {
      public:
        column_bases() = default;
        column_bases(const column_bases&) = default;
        column_bases(column_bases&&) = default;
        column_bases& operator=(const column_bases&) = default;
        column_bases& operator=(column_bases&&) = default;

        bool empty() const { return data_.empty(); }
        double operator[](serum_index aSerumNo) const { return data_[aSerumNo.get()]; }
        serum_index size() const { return serum_index{data_.size()}; }

        void set(serum_index aSerumNo, double column_basis) { data_[aSerumNo.get()] = column_basis; }
        // void remove(const ReverseSortedIndexes& indexes, ReverseSortedIndexes::difference_type base_index = 0) { ae::chart::v2::remove(indexes, data_, base_index); }
        void insert(serum_index before, double value) { data_.insert(data_.begin() + static_cast<decltype(data_)::difference_type>(before.get()), value); }
        void remove(serum_index sr_no) { data_.erase(std::next(data_.begin(), *sr_no)); }

        // import from ace
        void add(double value) { data_.push_back(value); }

        auto begin() const { return data_.begin(); }
        auto end() const { return data_.end(); }
        const auto& data() const { return data_; }

      private:
        std::vector<double> data_{};

    }; // class column_bases

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v3::column_bases> : fmt::formatter<ae::fmt_helper::float_formatter> {
    auto format(const ae::chart::v3::column_bases& cb, format_context& ctx) const {
        fmt::format_to(ctx.out(), "[");
        for (const auto sr_no : cb.size()) {
            if (sr_no != ae::serum_index{0})
                fmt::format_to(ctx.out(), " ");
            format_val(cb[sr_no], ctx);
        }
        return fmt::format_to(ctx.out(), "]");
    }
};

template <> struct fmt::formatter<std::shared_ptr<ae::chart::v3::column_bases>> : fmt::formatter<ae::chart::v3::column_bases> {
    auto format(const std::shared_ptr<ae::chart::v3::column_bases>& cb, format_context& ctx) const {
        if (cb)
            return fmt::formatter<ae::chart::v3::column_bases>::format(*cb, ctx);
        else
            return fmt::format_to(ctx.out(), "<none>");
    }
};

template <> struct fmt::formatter<ae::chart::v3::minimum_column_basis> : fmt::formatter<std::string> {
    auto format(const ae::chart::v3::minimum_column_basis& mcb, format_context& ctx) const { return fmt::formatter<std::string>::format(static_cast<std::string>(mcb), ctx); }
};

// ----------------------------------------------------------------------
