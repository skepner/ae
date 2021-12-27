#pragma once

#include <variant>
#include <limits>
#include <regex>
#include <optional>

#include "ext/fmt.hh"
#include "ext/date.hh"
#include "utils/named-type.hh"

// ----------------------------------------------------------------------

namespace ae::xlsx::inline v1
{

    namespace cell
    {
        class empty
        {
        };
        class error
        {
        };
    } // namespace cell

    using cell_t = std::variant<cell::empty, cell::error, bool, std::string, double, long, std::chrono::year_month_day>;

    inline bool is_empty(const cell_t& cell)
    {
        return std::visit(
            []<typename Content>(const Content&) {
                if constexpr (std::is_same_v<Content, cell::empty>)
                    return true;
                else
                    return false;
            },
            cell);
    }

    inline bool is_date(const cell_t& cell)
    {
        return std::visit(
            []<typename Content>(const Content&) {
                if constexpr (std::is_same_v<Content, std::chrono::year_month_day>)
                    return true;
                else
                    return false;
            },
            cell);
    }

    inline bool is_string(const cell_t& cell)
    {
        return std::visit(
            []<typename Content>(const Content&) {
                if constexpr (std::is_same_v<Content, std::string>)
                    return true;
                else
                    return false;
            },
            cell);
    }

    // ----------------------------------------------------------------------

    // struct cell_span_t
    // {
    //     size_t first;
    //     size_t last;
    //     std::string foreground{};
    //     std::string background{};
    // };

    // using cell_spans_t = std::vector<cell_span_t>;

    // ----------------------------------------------------------------------

    constexpr const auto max_row_col = std::numeric_limits<size_t>::max();

    using nrow_t = named_size_t<struct nrow_t_tag>;
    using ncol_t = named_size_t<struct ncol_t_tag>;

    template <typename nrowcol> concept NRowCol = std::is_same_v<nrowcol, nrow_t> || std::is_same_v<nrowcol, ncol_t>;

    template <NRowCol nrowcol> constexpr bool valid(nrowcol row_col) { return row_col != nrowcol{max_row_col}; }

    struct cell_addr_t
    {
        nrow_t row{max_row_col};
        ncol_t col{max_row_col};
    };

    struct cell_match_t
    {
        nrow_t row{max_row_col};
        ncol_t col{max_row_col};
        std::vector<std::string> matches{}; // match groups starting with 0
    };


    template <NRowCol nrowcol> struct range : public std::pair<nrowcol, nrowcol>
    {
        range() : std::pair<nrowcol, nrowcol>{max_row_col, max_row_col} {}

        constexpr bool valid() const { return this->first != nrowcol{max_row_col} && this->first <= this->second; }
        constexpr bool empty() const { return !valid(); }
        constexpr size_t length() const { return valid() ? *this->second - *this->first + 1 : 0; }
    };

    using row_range = range<nrow_t>;
    using column_range = range<ncol_t>;

    class Sheet
    {
      public:
        virtual ~Sheet() = default;

        virtual std::string name() const = 0;
        virtual nrow_t number_of_rows() const = 0;
        virtual ncol_t number_of_columns() const = 0;
        virtual cell_t cell(nrow_t row, ncol_t col) const = 0;                               // row and col are zero based
        // virtual cell_spans_t cell_spans(nrow_t /*row*/, ncol_t /*col*/) const { return {}; } // row and col are zero based

        static bool matches(const std::regex& re, const cell_t& cell);
        static bool matches(const std::regex& re, std::smatch& match, const cell_t& cell);
        bool matches(const std::regex& re, nrow_t row, ncol_t col) const { return matches(re, cell(row, col)); }
        bool is_date(nrow_t row, ncol_t col) const { return ae::xlsx::is_date(cell(row, col)); }
        size_t size(const cell_t& cell) const;
        size_t size(nrow_t row, ncol_t col) const { return size(cell(row, col)); }

        bool maybe_titer(const cell_t& cell) const;
        bool maybe_titer(nrow_t row, ncol_t col) const { return maybe_titer(cell(row, col)); }
        column_range titer_range(nrow_t row) const; // returns column range, returns empty range if not found

        cell_addr_t min_cell() const { return {nrow_t{0ul}, ncol_t{0ul}}; }
        cell_addr_t max_cell() const { return {number_of_rows(), number_of_columns()}; }

        std::vector<cell_match_t> grep(const std::regex& rex, const cell_addr_t& min, const cell_addr_t& max) const;

        // finds sets of two cells, the second one is right below the the first one
        // returns references to the second cells
        std::vector<cell_match_t> grepv(const std::regex& rex1, const std::regex& rex2, const cell_addr_t& min, const cell_addr_t& max) const;
    };

} // namespace ae::xlsx::inline v1

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::xlsx::cell_t> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::xlsx::cell_t& cell, FormatCtx& ctx)
    {
        std::visit(
            [&ctx]<typename Content>(const Content& arg) {
                if constexpr (std::is_same_v<Content, ae::xlsx::cell::empty>)
                    ; // format_to(ctx.out(), "<empty>");
                else if constexpr (std::is_same_v<Content, ae::xlsx::cell::error>)
                    format_to(ctx.out(), "<error>");
                else if constexpr (std::is_same_v<Content, bool>)
                    format_to(ctx.out(), "{}", arg);
                else if constexpr (std::is_same_v<Content, std::string> || std::is_same_v<Content, double> || std::is_same_v<Content, long>)
                    format_to(ctx.out(), "{}", arg);
                else if constexpr (std::is_same_v<Content, std::chrono::year_month_day>)
                    format_to(ctx.out(), "{}", arg);
                else
                    format_to(ctx.out(), "<*unknown*>");
            },
            cell);
        return ctx.out();
    }
};

template <> struct fmt::formatter<ae::xlsx::nrow_t> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(ae::xlsx::nrow_t row, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "{}", *row + 1);
    }
};

template <> struct fmt::formatter<ae::xlsx::ncol_t> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(ae::xlsx::ncol_t col, FormatCtx& ctx)
    {
        auto coll = *col;
        std::string nn;
        while (true) {
            nn.append(1, (coll % 26) + 'A');
            if (coll < 26)
                break;
            coll = coll / 26 - 1;
        }
        std::reverse(std::begin(nn), std::end(nn));
        return format_to(ctx.out(), "{}", nn);
    }
};

template <> struct fmt::formatter<ae::xlsx::cell_addr_t> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::xlsx::cell_addr_t& addr, FormatCtx& ctx) { return format_to(ctx.out(), "{}{}", addr.col, addr.row); }
};

template <> struct fmt::formatter<ae::xlsx::cell_match_t> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::xlsx::cell_match_t& match, FormatCtx& ctx) { return format_to(ctx.out(), "{}{}:{}", match.row, match.col, match.matches); }
};

template <ae::xlsx::NRowCol nrowcol> struct fmt::formatter<ae::xlsx::range<nrowcol>> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::xlsx::range<nrowcol>& rng, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "{}:{}", rng.first, rng.second);
    }
};

template <ae::xlsx::NRowCol nrowcol> struct fmt::formatter<std::optional<nrowcol>> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(std::optional<nrowcol> rowcol, FormatCtx& ctx)
    {
        if (rowcol.has_value())
            return format_to(ctx.out(), "{}", *rowcol);
        else
            return format_to(ctx.out(), "{}", "**no-value**");
    }
};

// ----------------------------------------------------------------------
