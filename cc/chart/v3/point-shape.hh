#pragma once

#include "ext/fmt.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class point_shape
    {
      public:
        enum Shape { Circle, Box, Triangle, Egg, UglyEgg };

        point_shape() noexcept {}
        point_shape(const point_shape&) noexcept = default;
        point_shape(Shape aShape) noexcept : mShape{aShape} {}
        explicit point_shape(std::string_view aShape) { from(aShape); }
        point_shape& operator=(const point_shape&) noexcept = default;
        point_shape& operator=(Shape aShape) noexcept
        {
            mShape = aShape;
            return *this;
        }
        point_shape& operator=(std::string_view aShape)
        {
            from(aShape);
            return *this;
        }
        [[nodiscard]] bool operator==(const point_shape& ps) const noexcept = default;

        operator Shape() const noexcept { return mShape; }
        Shape get() const noexcept { return mShape; }

      private:
        Shape mShape{Circle};

        void from(std::string_view aShape)
        {
            if (!aShape.empty()) {
                switch (aShape.front()) {
                    case 'C':
                    case 'c':
                        mShape = Circle;
                        break;
                    case 'E':
                    case 'e':
                        mShape = Egg;
                        break;
                    case 'U':
                    case 'u':
                        mShape = UglyEgg;
                        break;
                    case 'B':
                    case 'b':
                    case 'R': // rectangle
                    case 'r':
                        mShape = Box;
                        break;
                    case 'T':
                    case 't':
                        mShape = Triangle;
                        break;
                    default:
                        std::runtime_error(fmt::format("Unrecognized point shape: {}", aShape));
                }
            }
            else {
                std::runtime_error("Unrecognized empty point shape");
            }
        }

    }; // class point_shape

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v3::point_shape> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::chart::v3::point_shape& shape, FormatCtx& ctx)
    {
        using namespace ae::chart::v3;
        switch (shape.get()) {
            case point_shape::Circle:
                return format_to(ctx.out(), "CIRCLE");
            case point_shape::Box:
                return format_to(ctx.out(), "BOX");
            case point_shape::Triangle:
                return format_to(ctx.out(), "TRIANGLE");
            case point_shape::Egg:
                return format_to(ctx.out(), "EGG");
            case point_shape::UglyEgg:
                return format_to(ctx.out(), "UGLYEGG");
        }
        return format_to(ctx.out(), "CIRCLE");
    }
};

// ----------------------------------------------------------------------
