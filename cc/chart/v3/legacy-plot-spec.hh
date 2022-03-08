#pragma once

#include "utils/named-vector.hh"
#include "chart/v3/index.hh"
#include "chart/v3/point-style.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3::legacy
{
    using DrawingOrder = ae::named_vector_t<point_index, struct chart_DrawingOrder_tag_t>;

        // size_t index_of(size_t aValue) const { return static_cast<size_t>(std::find(begin(), end(), aValue) - begin()); }

        // void raise(size_t aIndex)
        // {
        //     if (const auto p = std::find(begin(), end(), aIndex); p != end())
        //         std::rotate(p, p + 1, end());
        // }

        // void raise(const std::vector<size_t>& aIndexes)
        // {
        //     std::for_each(aIndexes.begin(), aIndexes.end(), [this](size_t index) { this->raise(index); });
        // }

        // void lower(size_t aIndex)
        // {
        //     if (const auto p = std::find(rbegin(), rend(), aIndex); p != rend())
        //         std::rotate(p, p + 1, rend());
        // }

        // void lower(const std::vector<size_t>& aIndexes)
        // {
        //     std::for_each(aIndexes.begin(), aIndexes.end(), [this](size_t index) { this->lower(index); });
        // }

        // void fill_if_empty(size_t aSize)
        // {
        //     if (get().empty()) {
        //         get().resize(aSize);
        //         std::iota(begin(), end(), 0);
        //     }
        // }

        // void insert(size_t before)
        // {
        //     std::for_each(begin(), end(), [before](size_t& point_no) {
        //         if (point_no >= before)
        //             ++point_no;
        //     });
        //     push_back(before);
        // }

        // void remove_points(const ReverseSortedIndexes& to_remove, size_t base_index = 0)
        // {
        //     for (const auto index : to_remove) {
        //         const auto real_index = index + base_index;
        //         remove(real_index);
        //         // if (const auto found = std::find(begin(), end(), real_index); found != end())
        //         //     erase(found);
        //         std::for_each(begin(), end(), [real_index](size_t& point_no) {
        //             if (point_no > real_index)
        //                 --point_no;
        //         });
        //     }
        // }

    // ----------------------------------------------------------------------

    class PlotSpec
    {
      public:
        PlotSpec() = default;
        PlotSpec(const PlotSpec&) = default;
        PlotSpec(PlotSpec&&) = default;
        PlotSpec& operator=(const PlotSpec&) = default;
        PlotSpec& operator=(PlotSpec&&) = default;

        // virtual size_t number_of_points() const = 0;
        // virtual bool empty() const = 0;
        // virtual PointStyle style(size_t aPointNo) const = 0;
        // virtual PointStyle& style_ref(size_t /*aPointNo*/) { throw std::runtime_error{"PointStyles::style_ref not supported for this style collection"}; }
        // virtual PointStylesCompacted compacted() const = 0;

        const DrawingOrder& drawing_order() const { return drawing_order_; }
        DrawingOrder& drawing_order() { return drawing_order_; }
        Color error_line_positive_color() const { return error_line_positive_color_; }
        void error_line_positive_color(Color color) { error_line_positive_color_ = color; }
        Color error_line_negative_color() const { return error_line_negative_color_; }
        void error_line_negative_color(Color color) { error_line_negative_color_ = color; }

        // virtual std::vector<acmacs::PointStyle> all_styles() const = 0;

      //   acmacs::PointStyle style(size_t aPointNo) const { return modified() ? style_modified(aPointNo) : main_->style(aPointNo); }
      //   std::vector<acmacs::PointStyle> all_styles() const { return modified() ? styles_ : main_->all_styles(); }
      //   size_t number_of_points() const { return modified() ? styles_.size() : main_->number_of_points(); }

      //   void raise(size_t point_no)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       drawing_order_.raise(point_no);
      //   }
      //   void raise(const Indexes& points)
      //   {
      //       modify();
      //       std::for_each(points.begin(), points.end(), [this](size_t index) { this->raise(index); });
      //   }
      //   void lower(size_t point_no)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       drawing_order_.lower(point_no);
      //   }
      //   void lower(const Indexes& points)
      //   {
      //       modify();
      //       std::for_each(points.begin(), points.end(), [this](size_t index) { this->lower(index); });
      //   }
      //   void raise_serum(size_t serum_no) { raise(serum_no + number_of_antigens_); }
      //   void raise_serum(const Indexes& sera)
      //   {
      //       std::for_each(sera.begin(), sera.end(), [this](size_t index) { this->raise(index + this->number_of_antigens_); });
      //   }
      //   void lower_serum(size_t serum_no) { lower(serum_no + number_of_antigens_); }
      //   void lower_serum(const Indexes& sera)
      //   {
      //       std::for_each(sera.begin(), sera.end(), [this](size_t index) { this->lower(index + this->number_of_antigens_); });
      //   }

      //   void shown(size_t point_no, bool shown)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       styles_[point_no].shown(shown);
      //   }
      //   void size(size_t point_no, ae::draw::v1::Pixels size)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       styles_[point_no].size(size);
      //   }
      //   void fill(size_t point_no, Color fill)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       styles_[point_no].fill(fill);
      //   }
      //   void fill_opacity(size_t point_no, double opacity)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       validate_opacity(opacity);
      //       styles_[point_no].fill(acmacs::color::Modifier{acmacs::color::Modifier::transparency_set{1.0 - opacity}});
      //   }
      //   void outline(size_t point_no, Color outline)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       styles_[point_no].outline(outline);
      //   }
      //   void outline_opacity(size_t point_no, double opacity)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       validate_opacity(opacity);
      //       styles_[point_no].outline(acmacs::color::Modifier{acmacs::color::Modifier::transparency_set{1.0 - opacity}});
      //   }
      //   void outline_width(size_t point_no, ae::draw::v1::Pixels outline_width)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       styles_[point_no].outline_width(outline_width);
      //   }
      //   void rotation(size_t point_no, ae::draw::v1::Rotation rotation)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       styles_[point_no].rotation(rotation);
      //   }
      //   void aspect(size_t point_no, ae::draw::v1::Aspect aspect)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       styles_[point_no].aspect(aspect);
      //   }
      //   void shape(size_t point_no, acmacs::PointShape::Shape shape)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       styles_[point_no].shape(shape);
      //   }
      //   void label_shown(size_t point_no, bool shown)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       styles_[point_no].label().shown = shown;
      //   }
      //   void label_offset_x(size_t point_no, double offset)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       styles_[point_no].label().offset.x(offset);
      //   }
      //   void label_offset_y(size_t point_no, double offset)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       styles_[point_no].label().offset.y(offset);
      //   }
      //   void label_size(size_t point_no, ae::draw::v1::Pixels size)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       styles_[point_no].label().size = size;
      //   }
      //   void label_color(size_t point_no, Color color)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       styles_[point_no].label().color.add(acmacs::color::Modifier{color});
      //   }
      //   void label_rotation(size_t point_no, ae::draw::v1::Rotation rotation)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       styles_[point_no].label().rotation = rotation;
      //   }
      //   void label_slant(size_t point_no, acmacs::FontSlant slant)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       styles_[point_no].label().style.slant = slant;
      //   }
      //   void label_weight(size_t point_no, acmacs::FontWeight weight)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       styles_[point_no].label().style.weight = weight;
      //   }
      //   void label_font_family(size_t point_no, std::string font_family)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       styles_[point_no].label().style.font_family = font_family;
      //   }
      //   void label_text(size_t point_no, std::string text)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       styles_[point_no].label_text(text);
      //   }

      //   void scale_all(double point_scale, double outline_scale)
      //   {
      //       modify();
      //       std::for_each(styles_.begin(), styles_.end(), [=](auto& style) { style.scale(point_scale).scale_outline(outline_scale); });
      //   }

      //   void modify(size_t point_no, const acmacs::PointStyleModified& style)
      //   {
      //       modify();
      //       validate_point_no(point_no);
      //       styles_[point_no] = style;
      //   }
      //   void modify(const Indexes& points, const acmacs::PointStyleModified& style)
      //   {
      //       modify();
      //       std::for_each(points.begin(), points.end(), [this, &style](size_t index) { this->modify(index, style); });
      //   }
      //   void modify_serum(size_t serum_no, const acmacs::PointStyleModified& style) { modify(serum_no + number_of_antigens_, style); }
      //   void modify_sera(const Indexes& sera, const acmacs::PointStyleModified& style)
      //   {
      //       std::for_each(sera.begin(), sera.end(), [this, &style](size_t index) { this->modify(index + this->number_of_antigens_, style); });
      //   }

      //   void remove_antigens(const ReverseSortedIndexes& indexes);
      //   void remove_sera(const ReverseSortedIndexes& indexes);
      //   void insert_antigen(size_t before);
      //   void append_antigen() { insert_antigen(number_of_antigens_); }
      //   void insert_serum(size_t before);

      // protected:
      //   virtual bool modified() const { return modified_; }
      //   virtual void modify()
      //   {
      //       if (!modified())
      //           clone_from(*main_);
      //   }
      //   void clone_from(const PlotSpec& aSource)
      //   {
      //       modified_ = true;
      //       styles_ = aSource.all_styles();
      //       drawing_order_ = aSource.drawing_order();
      //       drawing_order_.fill_if_empty(number_of_points());
      //   }
      //   const acmacs::PointStyle& style_modified(size_t point_no) const { return styles_.at(point_no); }

      //   void validate_point_no(size_t point_no) const
      //   {
      //       // std::cerr << "DEBUG: PlotSpecModify::validate_point_no: number_of_points main: " << main_->number_of_points() << " modified: " << modified() << " number_of_points: " <<
      //       // number_of_points() << '\n';
      //       if (point_no >= number_of_points())
      //           throw std::runtime_error{fmt::format("Invalid point number: {}, expected integer in range 0..{} inclusive", point_no, number_of_points() - 1)};
      //   }

      //   void validate_opacity(double opacity) const
      //   {
      //       if (opacity < 0 || opacity > 1.0)
      //           throw std::runtime_error{fmt::format("Invalid color opacity: {}, expected within range 0.0..1.0 inclusive", opacity)};
      //   }

      private:
        // antigen_index number_of_antigens_{};
        std::vector<PointStyle> styles_{};
        DrawingOrder drawing_order_{};
        Color error_line_positive_color_{"blue"};
        Color error_line_negative_color_{"red"};
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
