#include "ext/fmt.hh"
#include "ext/from_chars.hh"
#include "chart/v2/column-bases.hh"

// ----------------------------------------------------------------------

void ae::chart::v2::MinimumColumnBasis::from(std::string_view value)
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
        throw std::runtime_error{fmt::format("Unrecognized minimum_column_basis value: {}", value)};

} // ae::chart::v2::chart::MinimumColumnBasis::from

// ----------------------------------------------------------------------

std::string ae::chart::v2::MinimumColumnBasis::format(std::string_view format, use_none un) const noexcept
{
    if (is_none()) {
        if (un == use_none::yes)
            return fmt::format(fmt::runtime(format), "none");
        else
            return {};
    }
    else
        return fmt::format(fmt::runtime(format), std::lround(std::exp2(value_) * 10.0));

} // ae::chart::v2::chart::MinimumColumnBasis::format

// ----------------------------------------------------------------------
