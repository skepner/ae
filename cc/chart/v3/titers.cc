#include "ext/from_chars.hh"
#include "chart/v3/titers.hh"

// ----------------------------------------------------------------------

std::string_view ae::chart::v3::Titer::validate(std::string_view titer)
{
    if (titer.empty())
        throw invalid_titer(titer);

    const auto just_digits = [titer](auto&& data) {
        if (!std::all_of(std::begin(data), std::end(data), [](auto val) { return std::isdigit(val); }))
            throw invalid_titer(titer);
    };

    switch (titer.front()) {
      case '*':
          if (titer.size() != 1)
              throw invalid_titer(titer);
          break;
      case '<':
      case '>':
      case '~':
          just_digits(titer.substr(1));
          break;
      default:
          just_digits(titer);
          break;
    }
    return titer;

} // ae::chart::v3::Titer::validate

// ----------------------------------------------------------------------

double ae::chart::v3::Titer::logged_with_thresholded() const
{
    switch (type()) {
      case Invalid:
      case Regular:
      case DontCare:
      case Dodgy:
          return logged();
      case LessThan:
          return logged() - 1;
      case MoreThan:
          return logged() + 1;
    }
    throw invalid_titer(*this); // for gcc 7.2

} // ae::chart::v3::Titer::logged_with_thresholded

// ----------------------------------------------------------------------

std::string ae::chart::v3::Titer::logged_as_string() const
{
    switch (type()) {
      case Invalid:
          throw invalid_titer(*this);
      case Regular:
          return fmt::format("{}", logged());
      case DontCare:
          return std::string{get()};
      case LessThan:
      case MoreThan:
      case Dodgy:
          return fmt::format("{}{}", get().front(), logged());
    }
    throw invalid_titer(*this); // for gcc 7.2

} // ae::chart::v3::Titer::logged_as_string

// ----------------------------------------------------------------------

double ae::chart::v3::Titer::logged_for_column_bases() const
{
    switch (type()) {
      case Invalid:
          throw invalid_titer(*this);
      case Regular:
      case LessThan:
          return logged();
      case MoreThan:
          return logged() + 1;
      case DontCare:
      case Dodgy:
          return -1;
    }
    throw invalid_titer(*this); // for gcc 7.2

} // ae::chart::v3::Titer::logged_for_column_bases

// ----------------------------------------------------------------------

size_t ae::chart::v3::Titer::value_for_sorting() const
{
    switch (type()) {
      case Invalid:
      case DontCare:
          return 0;
      case Regular:
          return from_chars<size_t>(get());
      case LessThan:
          return from_chars<size_t>(get().substr(1)) - 1;
      case MoreThan:
          return from_chars<size_t>(get().substr(1)) + 1;
      case Dodgy:
          return from_chars<size_t>(get().substr(1));
    }
    return 0;

} // ae::chart::v3::Titer::value_for_sorting

// ----------------------------------------------------------------------

size_t ae::chart::v3::Titer::value() const
{
    switch (type()) {
      case Invalid:
      case DontCare:
          return 0;
      case Regular:
          return from_chars<size_t>(get());
      case LessThan:
      case MoreThan:
      case Dodgy:
          return from_chars<size_t>(get().substr(1));
    }
    return 0;

} // ae::chart::v3::Titer::value

// ----------------------------------------------------------------------

size_t ae::chart::v3::Titer::value_with_thresholded() const
{
    switch (type()) {
      case Invalid:
      case DontCare:
          return 0;
      case Regular:
          return from_chars<size_t>(get());
      case LessThan:
          return from_chars<size_t>(get().substr(1)) / 2;
      case MoreThan:
          return from_chars<size_t>(get().substr(1)) * 2;
      case Dodgy:
          return from_chars<size_t>(get().substr(1));
    }
    return 0;

} // ae::chart::v3::Titer::value_with_thresholded

// ----------------------------------------------------------------------

ae::chart::v3::Titer ae::chart::v3::Titer::multiplied_by(double value) const // multiplied_by(2) returns 80 for 40 and <80 for <40, * for *
{
    switch (type()) {
      case Invalid:
      case DontCare:
          return *this;
      case Regular:
          return Titer{fmt::format("{}", std::lround(static_cast<double>(std::stoul(get())) * value))};
      case LessThan:
      case MoreThan:
      case Dodgy:
          return Titer{fmt::format("{}{}", get().front(), std::lround(static_cast<double>(std::stoul(get().substr(1))) * value))};
    }
    return Titer{};

} // ae::chart::v3::Titer::multiplied_by

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
