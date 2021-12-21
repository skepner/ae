#pragma once

#include <array>
#include "ext/fmt.hh"

// ----------------------------------------------------------------------

namespace acmacs::color
{
    struct error : public std::runtime_error { using std::runtime_error::runtime_error; };
}

// ----------------------------------------------------------------------

class Color
{
  public:
    constexpr Color() noexcept : color_{0xFF00FF} {}
    constexpr Color(const Color&) noexcept = default;
    template <typename Uint, typename std::enable_if_t<std::is_integral_v<Uint>>* = nullptr> constexpr Color(Uint aColor) noexcept : color_(static_cast<uint32_t>(aColor)) {}
    Color(std::string_view src) { from_string(src); }
    explicit Color(const char* src) : Color{std::string_view{src}} {}

    constexpr Color& operator=(const Color&) noexcept = default;
    Color& operator=(std::string_view src) { from_string(src); return *this; }
    Color& operator=(const char* src) { from_string(src); return *this; }
    template <typename Uint, typename std::enable_if_t<std::is_integral_v<Uint>>* = nullptr> constexpr Color& operator=(Uint aColor) noexcept { color_ = static_cast<uint32_t>(aColor); return *this; }

    constexpr bool operator == (const Color& aColor) const noexcept { return color_ == aColor.color_; }
    constexpr bool operator != (const Color& aColor) const noexcept { return ! operator==(aColor); }
    constexpr bool operator < (const Color& aColor) const noexcept { return color_ < aColor.color_; }

    constexpr double alpha() const noexcept { return double(0xFF - ((color_ >> 24) & 0xFF)) / 255.0; } // 0.0 - transparent, 1.0 - opaque
    constexpr double transparency() const noexcept { return double((color_ >> 24) & 0xFF) / 255.0; } // 1.0 - transparent, 0.0 - opaque
    constexpr double opacity() const noexcept { return alpha(); }
    constexpr double red() const noexcept { return double((color_ >> 16) & 0xFF) / 255.0; }
    constexpr double green() const noexcept { return double((color_ >> 8) & 0xFF) / 255.0; }
    constexpr double blue() const noexcept { return double(color_ & 0xFF) / 255.0; }

    constexpr uint32_t alphaI() const noexcept { return static_cast<uint32_t>((color_ >> 24) & 0xFF); }
    constexpr uint32_t redI() const noexcept { return static_cast<uint32_t>((color_ >> 16) & 0xFF); }
    constexpr uint32_t greenI() const noexcept { return static_cast<uint32_t>((color_ >> 8) & 0xFF); }
    constexpr uint32_t blueI() const noexcept { return static_cast<uint32_t>(color_ & 0xFF); }
    constexpr Color& alphaI(uint32_t v) noexcept { color_ = (color_ & 0xFFFFFF) | ((v & 0xFF) << 24); return *this; }
    constexpr uint32_t rgbI() const noexcept { return static_cast<uint32_t>(color_ & 0xFFFFFF); }

    constexpr void transparency(double transparency) noexcept { color_ = (color_ & 0x00FFFFFF) | ((static_cast<unsigned>(transparency * 255.0) & 0xFF) << 24); }
    constexpr void opacity(double opacity) noexcept { transparency(1.0 - opacity); }
    constexpr Color without_transparency() const noexcept { return {color_ & 0x00FFFFFF}; }

    constexpr bool is_opaque() const noexcept { return (color_ & 0xFF000000) == 0; }
    constexpr bool is_transparent() const noexcept { return !is_opaque(); }

    constexpr uint32_t raw_value() const noexcept { return color_; }
    std::string_view name() const noexcept;

  private:
    uint32_t color_;

    void from_string(std::string_view src);

};                              // class Color

// ----------------------------------------------------------------------

constexpr const Color BLACK{0};
constexpr const Color WHITE{0xFFFFFF};
constexpr const Color GREY{0xBEBEBE};
constexpr const Color GREY50{0x7F7F7F};
constexpr const Color GREY97{0xF7F7F7};
constexpr const Color TRANSPARENT{0xFF000000};

constexpr const Color BLUE{0x0000FF};
constexpr const Color CYAN{0x00FFFF};
constexpr const Color GREEN{0x00FF00};
constexpr const Color MAGENTA{0xFF00FF};
constexpr const Color ORANGE{0xFFA500};
constexpr const Color PINK{0xFFC0CB};
constexpr const Color PURPLE{0x800080};
constexpr const Color RED{0xFF0000};
constexpr const Color YELLOW{0xFFFF00};

// ----------------------------------------------------------------------

template <> struct fmt::formatter<Color>
{
    template <typename ParseContext> constexpr auto parse(ParseContext& ctx)
    {
        auto it = ctx.begin();
        if (it != ctx.end() && *it == ':')
            ++it;
        if (it != ctx.end() && *it != '}') {
            format_code_ = *it;
            ++it;
        }
        return std::find(it, ctx.end(), '}');
    }

    template <typename FormatCtx> auto format(const Color& val, FormatCtx& ctx)
    {
        switch (format_code_) {
          case 'X':
          case '#':
              return format_to(ctx.out(), "#{:06X}", val.raw_value());
          case 'x':
              return format_to(ctx.out(), "#{:06x}", val.raw_value());
          case 'c':
          case 'n':
              if (const auto name = val.name(); !name.empty())
                  return format_to(ctx.out(), "{}", name);
              else if (val.is_transparent())
                  return format_to(ctx.out(), "rgba({},{},{},{:.3f})", val.redI(), val.greenI(), val.blueI(), val.opacity());
              return format_to(ctx.out(), "#{:06x}", val.raw_value());
          case 'a':
          case 'r':
              return format_to(ctx.out(), "rgba({},{},{},{:.3f})", val.redI(), val.greenI(), val.blueI(), val.opacity());
          default:
              fmt::print(stderr, "WARNING unrecognized Color format code '{}', 'X' assumed\n", format_code_);
              return format_to(ctx.out(), "#{:06X}", val.raw_value());
        }
        return format_to(ctx.out(), "#{:06x}", val.raw_value()); // to avoid compiler warning
    }

  protected:
    constexpr char format_code() const noexcept { return format_code_; }

  private:
    char format_code_{'c'};
};

// ----------------------------------------------------------------------
