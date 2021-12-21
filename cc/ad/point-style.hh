#pragma once

#include "ad/text-style.hh"

// ----------------------------------------------------------------------

namespace acmacs
{
    class PointShape
    {
     public:
        enum Shape {Circle, Box, Triangle, Egg, UglyEgg};

        PointShape() noexcept : mShape{Circle} {}
        PointShape(const PointShape&) noexcept = default;
        PointShape(Shape aShape) noexcept : mShape{aShape} {}
        PointShape(std::string_view aShape) { from(aShape); }
        PointShape(const char* aShape) { from(aShape); }
        PointShape& operator=(const PointShape&) noexcept = default;
        PointShape& operator=(Shape aShape) noexcept { mShape = aShape; return *this; }
        PointShape& operator=(std::string_view aShape) { from(aShape); return *this; }
        [[nodiscard]] bool operator==(const PointShape& ps) const noexcept { return mShape == ps.mShape; }
        [[nodiscard]] bool operator!=(const PointShape& ps) const noexcept { return mShape != ps.mShape; }

        constexpr operator Shape() const noexcept { return mShape; }
        constexpr Shape get() const noexcept { return mShape; }

     private:
        Shape mShape;

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

    }; // class PointShape

    // ----------------------------------------------------------------------

    class PointStyleModified;

    class PointStyle
    {
      public:
        virtual ~PointStyle() = default;
        PointStyle() = default;
        PointStyle(const PointStyle&) = default;
        PointStyle(PointStyle&&) = default;
        PointStyle& operator=(const PointStyleModified& src);
        PointStyle& operator=(const PointStyle&) = default;
        PointStyle& operator=(PointStyle&&) = default;

        PointStyle& scale(double aScale) noexcept { size(size() * aScale); return *this; }
        PointStyle& scale_outline(double aScale) noexcept { outline_width(outline_width() * aScale); return *this; }

        [[nodiscard]] bool operator==(const PointStyle& rhs) const noexcept
        {
            return shown() == rhs.shown() && fill() == rhs.fill() && outline() == rhs.outline() && outline_width() == rhs.outline_width() && size() == rhs.size() && rotation() == rhs.rotation() &&
                   aspect() == rhs.aspect() && shape() == rhs.shape() && label() == rhs.label() && label_text() == rhs.label_text();
        }
        [[nodiscard]] bool operator!=(const PointStyle& rhs) const noexcept { return !operator==(rhs); }

        virtual bool shown() const noexcept { return shown_; }
        virtual Color fill() const noexcept { return fill_; }
        virtual Color outline() const noexcept { return outline_; }
        virtual Pixels outline_width() const noexcept { return outline_width_; }
        virtual Pixels size() const noexcept { return size_; }
        virtual Scaled diameter() const noexcept { return diameter_; } // drawi: use it if >0
        virtual Rotation rotation() const noexcept { return rotation_; }
        virtual Aspect aspect() const noexcept { return aspect_; }
        virtual PointShape shape() const noexcept { return shape_; }
        virtual const LabelStyle& label() const noexcept { return label_; }
        virtual std::string_view label_text() const noexcept { return label_text_; }

        virtual void fill(Color a_fill) noexcept { fill_ = a_fill; }
        virtual void fill(const acmacs::color::Modifier& a_fill) noexcept { acmacs::color::modify(fill_, a_fill); }
        virtual void outline(Color a_outline) noexcept { outline_ = a_outline; }
        virtual void outline(const acmacs::color::Modifier& a_outline) noexcept { acmacs::color::modify(outline_, a_outline); }

        virtual void shown(bool a_shown) noexcept { shown_ = a_shown; }
        virtual void outline_width(Pixels a_outline_width) noexcept { outline_width_ = a_outline_width; }
        virtual void size(Pixels a_size) noexcept { size_ = a_size; }
        virtual void radius(Scaled a_radius) noexcept { diameter_ = a_radius * 2.0; }
        virtual void rotation(Rotation a_rotation) noexcept { rotation_ = a_rotation; }
        virtual void aspect(Aspect a_aspect) noexcept { aspect_ = a_aspect; }
        virtual void shape(PointShape a_shape) noexcept { shape_ = a_shape; }
        virtual LabelStyle& label() noexcept { return label_; }
        virtual void label_text(std::string_view a_label_text) noexcept { label_text_.assign(a_label_text); }

      private:
        bool shown_{true};
        Color fill_{TRANSPARENT};
        Color outline_{BLACK};
        Pixels outline_width_{1.0};
        Pixels size_{5.0};
        Scaled diameter_{0.0}; // drawi: use it if >0
        Rotation rotation_{NoRotation};
        Aspect aspect_{AspectNormal};
        PointShape shape_;
        LabelStyle label_;
        std::string label_text_;

    }; // class PointStyle

    // ----------------------------------------------------------------------

    class PointStyleModified : public PointStyle
    {
      public:
        using PointStyle::PointStyle;
        PointStyleModified(PointStyle&& style) : PointStyle(std::move(style)) { all_modified(); }
        PointStyleModified(const PointStyle& style) = delete;

        using PointStyle::shown;
        using PointStyle::outline_width;
        using PointStyle::size;
        using PointStyle::rotation;
        using PointStyle::aspect;
        using PointStyle::shape;
        using PointStyle::label;
        using PointStyle::label_text;

        void fill(const acmacs::color::Modifier& a_fill) noexcept override { fill_modifier_.add(a_fill); }
        void outline(const acmacs::color::Modifier& a_outline) noexcept override { outline_modifier_.add(a_outline); }

        Color fill() const noexcept override { auto fl = PointStyle::fill(); return acmacs::color::modify(fl, fill_modifier_); }
        Color outline() const noexcept override { auto outl = PointStyle::outline(); return acmacs::color::modify(outl, outline_modifier_); }

        constexpr const auto& fill_modifier() const { return fill_modifier_; }
        constexpr const auto& outline_modifier() const { return outline_modifier_; }

        void shown(bool a_shown) noexcept override { PointStyle::shown(a_shown); modified_shown_ = true; }
        void outline_width(Pixels a_outline_width) noexcept override { PointStyle::outline_width(a_outline_width); modified_outline_width_ = true; }
        void size(Pixels a_size) noexcept override { PointStyle::size(a_size); modified_size_ = true; }
        void rotation(Rotation a_rotation) noexcept override { PointStyle::rotation(a_rotation); modified_rotation_ = true; }
        void aspect(Aspect a_aspect) noexcept override { PointStyle::aspect(a_aspect); modified_aspect_ = true; }
        void shape(PointShape a_shape) noexcept override { PointStyle::shape(a_shape); modified_shape_ = true; }
        LabelStyle& label() noexcept override { modified_label_ = true; return PointStyle::label(); }
        void label_text(std::string_view a_label_text) noexcept override { PointStyle::label_text(a_label_text); modified_label_text_ = true; }

        constexpr bool modified() const
        {
            return modified_shown_ || modified_outline_width_ || modified_size_ || modified_rotation_ || modified_aspect_ || modified_shape_ || modified_label_ || modified_label_text_;
        }

      private:
        bool modified_shown_{false};
        bool modified_outline_width_{false};
        bool modified_size_{false};
        bool modified_rotation_{false};
        bool modified_aspect_{false};
        bool modified_shape_{false};
        bool modified_label_{false};
        bool modified_label_text_{false};

        acmacs::color::Modifier fill_modifier_;
        acmacs::color::Modifier outline_modifier_;

        void all_modified() noexcept
        {
            modified_shown_ = true;
            modified_outline_width_ = true;
            modified_size_ = true;
            modified_rotation_ = true;
            modified_aspect_ = true;
            modified_shape_ = true;
            modified_label_ = true;
            modified_label_text_ = true;
        }

        friend class PointStyle;
    };

    // ----------------------------------------------------------------------

    inline PointStyle& PointStyle::operator=(const PointStyleModified& src)
    {
        if (!src.fill_modifier_.is_no_change()) fill(src.fill_modifier_);
        if (!src.outline_modifier_.is_no_change()) outline(src.outline_modifier_);

        if (src.modified_shown_) shown(src.shown());
        if (src.modified_outline_width_) outline_width(src.outline_width());
        if (src.modified_size_) size(src.size());
        if (src.modified_rotation_) rotation(src.rotation());
        if (src.modified_aspect_) aspect(src.aspect());
        if (src.modified_shape_) shape(src.shape());
        if (src.modified_label_) label() = src.label();
        if (src.modified_label_text_) label_text(src.label_text());
        return *this;
    }

    // ----------------------------------------------------------------------

    struct PointStylesCompacted
    {
        std::vector<PointStyle> styles;
        std::vector<size_t> index;

    }; // class PointStylesCompacted

    class PointStyles
    {
     public:
        constexpr PointStyles() noexcept = default;
        constexpr PointStyles(const PointStyles&) noexcept = delete;
        virtual ~PointStyles() = default;

        virtual size_t number_of_points() const = 0;
        virtual bool empty() const = 0;
        virtual PointStyle style(size_t aPointNo) const = 0;
        virtual PointStyle& style_ref(size_t /*aPointNo*/) { throw std::runtime_error{"PointStyles::style_ref not supported for this style collection"}; }
        virtual PointStylesCompacted compacted() const = 0;

    }; // class PointStyles

} // namespace acmacs

// ----------------------------------------------------------------------

template <> struct fmt::formatter<acmacs::PointShape> : fmt::formatter<acmacs::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const acmacs::PointShape& shape, FormatCtx& ctx)
    {
        switch (shape.get()) {
            case acmacs::PointShape::Circle:
                return format_to(ctx.out(), "CIRCLE");
            case acmacs::PointShape::Box:
                return format_to(ctx.out(), "BOX");
            case acmacs::PointShape::Triangle:
                return format_to(ctx.out(), "TRIANGLE");
            case acmacs::PointShape::Egg:
                return format_to(ctx.out(), "EGG");
            case acmacs::PointShape::UglyEgg:
                return format_to(ctx.out(), "UGLYEGG");
        }
        return format_to(ctx.out(), "CIRCLE");
    }
};

template <> struct fmt::formatter<acmacs::PointStyle> : fmt::formatter<acmacs::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const acmacs::PointStyle& style, FormatCtx& ctx)
    {
        return format_to(ctx.out(), R"({{"shape": {}, "shown": {}, "fill": "{}", "outline": "{}", "outline_width": {}, "size": {}, "aspect": {}, "rotation": {}, "label": {}, "label_text": "{}"}})",
                         style.shape(), style.shown(), style.fill(), style.outline(), style.outline_width(), style.size(), style.aspect(), style.rotation(), style.label(), style.label_text());
    }
};

template <> struct fmt::formatter<acmacs::PointStyleModified> : fmt::formatter<acmacs::PointStyle>
{
};

// ----------------------------------------------------------------------
