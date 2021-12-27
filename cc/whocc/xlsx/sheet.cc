#include "ext/range-v3.hh"
#include "whocc/xlsx/sheet.hh"
#include "utils/log.hh"

// ----------------------------------------------------------------------

bool ae::xlsx::v1::Sheet::matches(const std::regex& re, const cell_t& cell)
{
    return std::visit(
        [&re, &cell]<typename Content>(const Content& arg) {
            if constexpr (std::is_same_v<Content, std::string>)
                return std::regex_search(arg, re);
            else
                return std::regex_search(fmt::format("{}", cell), re); // CDC id is a number in CDC tables, still we want to match
        },
        cell);

} // ae::xlsx::v1::Sheet::matches

// ----------------------------------------------------------------------

bool ae::xlsx::v1::Sheet::matches(const std::regex& re, std::smatch& match, const cell_t& cell)
{
    return std::visit(
        [&re, &match]<typename Content>(const Content& arg) {
            if constexpr (std::is_same_v<Content, std::string>)
                return std::regex_search(arg, match, re);
            else
                return false;
        },
        cell);

} // ae::xlsx::v1::Sheet::matches

// ----------------------------------------------------------------------

size_t ae::xlsx::v1::Sheet::size(const cell_t& cell) const
{
    return std::visit(
        []<typename Content>(const Content& arg) {
            if constexpr (std::is_same_v<Content, std::string>)
                return arg.size();
            else
                return 0ul;
        },
        cell);

} // ae::xlsx::v1::Sheet::size

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

// "\xEF\xBC\x9C" -> "<" unicode Fullwidth Less-Than Sign &#xFF1C; (NIID)
// ">" and "," - perhaps typos in Crick tables
static const std::regex re_titer{R"(^\s*(<|>|,|(?:<|>|\xEF\xBC\x9C)?\s*[1-9][0-9]{0,5}|N[DAT]|QNS|\*)\s*$)", std::regex::icase | std::regex::ECMAScript | std::regex::optimize};

#pragma GCC diagnostic pop

bool ae::xlsx::v1::Sheet::maybe_titer(const cell_t& cell) const
{
    return std::visit(
        []<typename Content>(const Content& arg) {
            if constexpr (std::is_same_v<Content, std::string>) {
                // if (!arg.empty() && static_cast<unsigned char>(arg[0]) > 0x7F) {
                //     AD_DEBUG("titer? \"{}\"", arg);
                //     for (auto cc : arg)
                //         AD_DEBUG("titer? 0x{:X}", static_cast<unsigned char>(cc));
                // }
                return std::regex_search(arg, re_titer);
            }
            else if constexpr (std::is_same_v<Content, double> || std::is_same_v<Content, long>)
                return arg > 0;
            else
                return false;
        },
        cell);

} // ae::xlsx::v1::Sheet::maybe_titer

// ----------------------------------------------------------------------

ae::xlsx::v1::column_range ae::xlsx::v1::Sheet::titer_range(nrow_t row) const
{
    range<ncol_t> longest;
    range<ncol_t> current;
    const auto update = [&longest, &current] {
        if (current.valid()) {
            if (!longest.valid() || longest.length() < current.length()) {
                longest = current;
                // AD_DEBUG("longest {}: {}", row, longest);
            }
            current = range<ncol_t>{};
        }
    };

    for (auto col = ncol_t{0}; col < number_of_columns(); ++col) {
        // AD_DEBUG("maybe_titer {}:{} \"{}\"", row, col, cell(row, col));
        if (maybe_titer(row, col)) {
            if (!current.valid())
                current.first = col;
            current.second = col;
            // AD_DEBUG("titer {}{}: {} --> {}", row, col, cell(row, col), current);
        }
        else
            update();
    }
    update();
    return longest;

} // ae::xlsx::v1::Sheet::titer_range

// ----------------------------------------------------------------------

std::vector<ae::xlsx::cell_match_t> ae::xlsx::v1::Sheet::grep(const std::regex& rex, const cell_addr_t& min, const cell_addr_t& max) const
{
    std::vector<cell_match_t> result;
    for (auto row = min.row; row < max.row; ++row) {
        for (auto col = min.col; col < max.col; ++col) {
            const auto cl = cell(row, col);
            // AD_DEBUG("xlsx::grep {} {} \"{}\"", row, col, cl);
            std::smatch match;
            if (matches(rex, match, cl)) {
                cell_match_t cm{.row = row, .col = col, .matches = std::vector<std::string>(match.size())};
                std::transform(std::cbegin(match), std::cend(match), std::begin(cm.matches), [](const auto& submatch) { return submatch.str(); });
                result.push_back(std::move(cm));
            }
        }
    }
    return result;

} // ae::xlsx::v1::Sheet::grep

// ----------------------------------------------------------------------

std::vector<ae::xlsx::cell_match_t> ae::xlsx::v1::Sheet::grepv(const std::regex& rex1, const std::regex& rex2, const cell_addr_t& min, const cell_addr_t& max) const
{
    std::vector<cell_match_t> result;
    for (auto row = min.row; row < max.row; ++row) {
        for (auto col = min.col; col < max.col; ++col) {
            const auto cl1 = cell(row, col);
            const auto cl2 = cell(row + nrow_t{1}, col);
            // AD_DEBUG("xlsx::grep {} {} \"{}\"", row, col, cl);
            std::smatch match;
            if (matches(rex1, cl1) && matches(rex2, match, cl2)) {
                cell_match_t cm{.row = row, .col = col, .matches = std::vector<std::string>(match.size())};
                std::transform(std::cbegin(match), std::cend(match), std::begin(cm.matches), [](const auto& submatch) { return submatch.str(); });
                result.push_back(std::move(cm));
            }
        }
    }
    return result;

} // ae::xlsx::v1::Sheet::grepv

// ----------------------------------------------------------------------
