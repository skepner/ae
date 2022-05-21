#pragma once

#include "ext/fmt.hh"
#include "utils/float.hh"
#include "draw/v2/color.hh"
#include "draw/v2/rotation.hh"

// ----------------------------------------------------------------------

namespace ae::draw::v2
{
    class font_slant_t
    {
     public:
        enum Value { Normal, Italic };

        font_slant_t(Value aFontSlant = Normal) noexcept : slant_{aFontSlant} {}
        font_slant_t(const font_slant_t&) noexcept = default;
        font_slant_t(std::string_view aFontSlant) { from(aFontSlant); }
        font_slant_t& operator=(const font_slant_t&) noexcept = default;
        font_slant_t& operator=(std::string_view aFontSlant) { from(aFontSlant); return *this; }

        constexpr operator Value() const noexcept { return slant_; }
        constexpr Value get() const noexcept { return slant_; }

     private:
        Value slant_{Normal};

        void from(std::string_view aFontSlant)
        {
            using namespace std::string_view_literals;
            if (aFontSlant == "normal"sv)
                slant_ = Normal;
            else if (aFontSlant == "italic"sv)
                slant_ = Italic;
            else
                std::runtime_error{fmt::format("Unrecognized slant: {}", aFontSlant)};
        }

    }; // class font_slant_t

    inline bool is_default(const font_slant_t& slant) { return slant == font_slant_t{}; }

// ----------------------------------------------------------------------

    class font_weight_t
    {
     public:
        enum Value { Normal, Bold };

        font_weight_t(Value font_weight = Normal) noexcept : weight_{font_weight} {}
        font_weight_t(const font_weight_t&) noexcept = default;
        font_weight_t(std::string_view font_weight) { from(font_weight); }
        font_weight_t& operator=(const font_weight_t&) noexcept = default;
        font_weight_t& operator=(std::string_view font_weight) { from(font_weight); return *this; }

        constexpr operator Value() const noexcept { return weight_; }
        constexpr Value get() const noexcept { return weight_; }

     private:
        Value weight_{Normal};

        void from(std::string_view font_weight)
        {
            using namespace std::string_view_literals;
            if (font_weight == "normal"sv)
                weight_ = Normal;
            else if (font_weight == "bold"sv)
                weight_ = Bold;
            else
                std::runtime_error(fmt::format("Unrecognized font weight: \"{}\"", font_weight));
        }

    }; // class font_weight_t

    inline bool is_default(const font_weight_t& weight) { return weight == font_weight_t{}; }

    // ----------------------------------------------------------------------

    struct text_style
    {
        text_style() = default;
        text_style(std::string_view font_name) : font_family{font_name} {}
        bool operator==(const text_style&) const = default;

        bool shown{true};
        Float size{16.0};
        Color color{"black"};
        font_slant_t slant{};
        font_weight_t weight{};
        std::string font_family{};
        Rotation rotation{NoRotation};
        Float interline{0.2};

    }; // class text_style

    inline bool is_default(const text_style& ts) { return ts == text_style{}; }

    // ----------------------------------------------------------------------

    struct text_data : public text_style
    {
        text_data() = default;
        bool operator==(const text_data&) const = default;

        std::optional<std::string> text{};
    };

    inline bool is_default(const text_data& td) { return td == text_data{}; }

    // ----------------------------------------------------------------------

    struct offset_t
    {
        Float x{0.0}, y{0.0};

        bool operator==(const offset_t&) const = default;
    };

    // ----------------------------------------------------------------------

    struct text_and_offset : public text_data
    {
        text_and_offset() = default;
        text_and_offset(offset_t&& offs) : offset{std::move(offs)} {}
        bool operator==(const text_and_offset&) const = default;

        offset_t offset{};
    };

    inline bool is_default(const text_and_offset& to) { return to == text_and_offset{}; }

    // ----------------------------------------------------------------------

    struct point_label : public text_and_offset
    {
        point_label() : text_and_offset(offset_t{0.0, 1.0}) {}
    };

    inline bool is_default(const point_label& pl) { return pl == point_label{}; }

} // namespace ae::draw::v2

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::draw::v2::font_slant_t> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> constexpr auto format(const ae::draw::v2::font_slant_t& slant, FormatCtx& ctx)
    {
        switch (slant.get()) {
            case ae::draw::v2::font_slant_t::Normal:
                return format_to(ctx.out(), "normal");
            case ae::draw::v2::font_slant_t::Italic:
                return format_to(ctx.out(), "italic");
        }
        return ctx.out();
    }
};

template <> struct fmt::formatter<ae::draw::v2::font_weight_t> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> constexpr auto format(const ae::draw::v2::font_weight_t& weight, FormatCtx& ctx)
    {
        switch (weight.get()) {
            case ae::draw::v2::font_weight_t::Normal:
                return format_to(ctx.out(), "normal");
            case ae::draw::v2::font_weight_t::Bold:
                return format_to(ctx.out(), "bold");
        }
        return ctx.out();
    }
};

template <> struct fmt::formatter<ae::draw::v2::text_style> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> constexpr auto format(const ae::draw::v2::text_style& style, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "shown:{}, color:\"{}\", slant:{}, weight:{} family:\"{}\", rotation:{}, interline:{}", style.shown, style.color, style.slant, style.weight, style.font_family,
                         style.rotation, style.interline);
    }
};

template <> struct fmt::formatter<ae::draw::v2::text_data> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> constexpr auto format(const ae::draw::v2::text_data& data, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "{}, text:{}", static_cast<const ae::draw::v2::text_style&>(data), data.text);
    }
};

template <> struct fmt::formatter<ae::draw::v2::offset_t> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> constexpr auto format(const ae::draw::v2::offset_t& offset, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "[{}, {}]", offset.x, offset.y);
    }
};

template <> struct fmt::formatter<ae::draw::v2::text_and_offset> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> constexpr auto format(const ae::draw::v2::text_and_offset& to, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "{}, offset:{}", static_cast<const ae::draw::v2::text_data&>(to), to.offset);
    }
};

// ----------------------------------------------------------------------
