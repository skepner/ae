#pragma once

#include <string>
#include <vector>
#include <numeric>
#include <algorithm>
#include <optional>

#include "utils/log.hh"
#include "ad/rjson-v2.hh"
#include "chart/v2/layout.hh"
#include "chart/v2/titers.hh"
#include "chart/v2/chart.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v2
{
    class TableDistances;
    class ColumnBases;
    class PointIndexList;

} // namespace ae::chart::v2


namespace ae::chart::v2::rjson_import
{
    inline number_of_dimensions_t number_of_dimensions(const rjson::value& data) noexcept
    {
        if (const auto& non_empty = rjson::find_if(data, [](const auto& val) -> bool { return !val.empty(); }); !non_empty.is_null())
            return number_of_dimensions_t{non_empty.size()};
        else
            return number_of_dimensions_t{0};
    }

// ----------------------------------------------------------------------

    class Layout : public ae::chart::v2::Layout
    {
     public:
        Layout(const rjson::value& aData);

    }; // class Layout

// ----------------------------------------------------------------------

    void update(const rjson::value& data, std::string_view list_key, const std::string& dict_key, TableDistances& table_distances, const ColumnBases& column_bases, const StressParameters& parameters, size_t number_of_points);

} // namespace ae::chart::v2::rjson

// ----------------------------------------------------------------------

namespace ae::chart::v2
{
    class RjsonTiters : public Titers
    {
      public:
        Titer titer(size_t aAntigenNo, size_t aSerumNo) const override
        {
            if (const auto& list = data_[keys_.list]; !list.is_null())
                return Titer{list[aAntigenNo][aSerumNo].to<std::string_view>()};
            else
                return titer_in_d(data_[keys_.dict], aAntigenNo, aSerumNo);
        }

        Titer titer_of_layer(size_t aLayerNo, size_t aAntigenNo, size_t aSerumNo) const override { return titer_in_d(data_[keys_.layers][aLayerNo], aAntigenNo, aSerumNo); }

        std::vector<Titer> titers_for_layers(size_t aAntigenNo, size_t aSerumNo, include_dotcare inc = include_dotcare::no) const override
        {
            return titers_for_layers(data_[keys_.layers], aAntigenNo, aSerumNo, inc);
        }

        std::vector<size_t> layers_with_antigen(size_t aAntigenNo) const override
        {
            return layers_with_antigen(data_[keys_.layers], aAntigenNo);
        }

        std::vector<size_t> layers_with_serum(size_t aSerumNo) const override
        {
            return layers_with_serum(data_[keys_.layers], aSerumNo);
        }

        size_t number_of_layers() const override { return data_[keys_.layers].size(); }

        size_t number_of_antigens() const override { return number_of_antigens_; }
        size_t number_of_sera() const override { return number_of_sera_; }

        size_t number_of_non_dont_cares() const override;
        size_t titrations_for_antigen(size_t antigen_no) const override;
        size_t titrations_for_serum(size_t serum_no) const override;

        // support for fast exporting into ace, if source was ace or acd1
        const rjson::value& rjson_list_list() const override
        {
            if (const auto& list = data_[keys_.list]; !list.empty())
                return list;
            else
                throw data_not_available{"no \"" + keys_.list + "\""};
        }
        const rjson::value& rjson_list_dict() const override
        {
            if (const auto& dict = data_[keys_.dict]; !dict.empty())
                return dict;
            else
                throw data_not_available{"no \"" + keys_.dict + "\""};
        }
        const rjson::value& rjson_layers() const override
        {
            if (const auto& layers = data_[keys_.layers]; !layers.empty())
                return layers;
            else
                throw data_not_available{"no \"" + keys_.layers + "\""};
        }

        void update(TableDistances& table_distances, const ColumnBases& column_bases, const StressParameters& parameters) const override
        {
            if (number_of_sera())
                rjson_import::update(data_, keys_.list, keys_.dict, table_distances, column_bases, parameters, number_of_antigens() + number_of_sera());
            else
                throw std::runtime_error(AD_FORMAT("genetic table support not implemented"));
        }

        TiterIteratorMaker titers_existing() const override;
        TiterIteratorMaker titers_existing_from_layer(size_t layer_no) const override;

      protected:
        struct Keys
        {
            std::string list;
            std::string dict;
            std::string layers;
        };

        // cannot correctly infer num of antigens and sera from titers when there are sera without titers at the end of the table
        RjsonTiters(const rjson::value& data, const Keys& keys, size_t number_of_antigens, size_t number_of_sera)
            : data_{data}, keys_{keys}, number_of_antigens_{number_of_antigens}, number_of_sera_{number_of_sera}
        {
        }

        // const rjson::value& data() const { return data_; }
        const rjson::value& layer(size_t aLayerNo) const { return rjson_layers()[aLayerNo]; }

        Titer titer_in_d(const rjson::value& aSource, size_t aAntigenNo, size_t aSerumNo) const
        {
            if (const auto& row = aSource[aAntigenNo]; !row.is_null())
                if (const auto& titer = row[aSerumNo]; !titer.is_null())
                    return Titer{titer.to<std::string_view>()};
            return {};
        }

        std::vector<Titer> titers_for_layers(const rjson::value& layers, size_t aAntigenNo, size_t aSerumNo, include_dotcare inc = include_dotcare::no) const;
        std::vector<size_t> layers_with_antigen(const rjson::value& layers, size_t aAntigenNo) const;
        std::vector<size_t> layers_with_serum(const rjson::value& layers, size_t aSerumNo) const;

        const rjson::value& data() const { return data_; }

      private:
        const rjson::value& data_;
        const Keys& keys_;
        const size_t number_of_antigens_;
        const size_t number_of_sera_;

    }; // class RjsonTiters

    // ----------------------------------------------------------------------

    class RjsonProjection : public Projection
    {
      public:
        std::optional<double> stored_stress() const override
        {
            if (const auto& stress = data_[keys_.stress]; !stress.is_null())
                return stress.to<double>();
            else
                return {};
        }
        std::shared_ptr<Layout> layout() const override
        {
            if (!layout_)
                layout_ = std::make_shared<rjson_import::Layout>(data_[keys_.layout]);
            return layout_;
        }
        number_of_dimensions_t number_of_dimensions() const override { return rjson_import::number_of_dimensions(data_[keys_.layout]); }
        std::string comment() const override
        {
            if (const auto& comment = data_[keys_.comment]; !comment.is_null())
                return comment.to<std::string>();
            else
                return {};
        }
        size_t number_of_points() const override { return data_[keys_.layout].size(); }
        DisconnectedPoints disconnected() const override;

      protected:
        struct Keys
        {
            std::string stress;
            std::string layout;
            std::string comment;
        };

        RjsonProjection(const Chart& chart, const rjson::value& data, const Keys& keys) : Projection(chart), data_{data}, keys_{keys} {}
        RjsonProjection(const Chart& chart, const rjson::value& data, const Keys& keys, size_t projection_no) : RjsonProjection(chart, data, keys) { set_projection_no(projection_no); }

        const rjson::value& data() const { return data_; }

        virtual DisconnectedPoints make_disconnected() const = 0;

      private:
        const rjson::value& data_;
        const Keys& keys_;
        mutable std::shared_ptr<Layout> layout_;

    }; // class RjsonProjection

} // namespace ae::chart::v2

// ----------------------------------------------------------------------
