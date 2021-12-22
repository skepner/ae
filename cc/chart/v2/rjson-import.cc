#include "ext/range-v3.hh"
#include "ext/from_chars.hh"
#include "chart/v2/rjson-import.hh"
#include "chart/v2/chart.hh"

// ----------------------------------------------------------------------

std::vector<ae::chart::v2::Titer> ae::chart::v2::RjsonTiters::titers_for_layers(const rjson::value& layers, size_t aAntigenNo, size_t aSerumNo, include_dotcare inc) const
{
    if (layers.empty())
        throw data_not_available("no layers");
    std::vector<Titer> result;
    rjson::for_each(layers, [&result, aAntigenNo, aSerumNo, inc](const rjson::value& layer) {
        if (const auto& for_ag = layer[aAntigenNo]; !for_ag.empty()) {
            if (const auto& titer = for_ag[aSerumNo]; !titer.is_null())
                result.emplace_back(titer.to<std::string_view>());
            else if (inc == include_dotcare::yes)
                result.push_back({});
        }
    });
    return result;

} // ae::chart::v2::RjsonTiters::titers_for_layers

// ----------------------------------------------------------------------

std::vector<size_t> ae::chart::v2::RjsonTiters::layers_with_antigen(const rjson::value& layers, size_t aAntigenNo) const
{
    if (layers.empty())
        throw data_not_available("no layers");
    std::vector<size_t> result;
    rjson::for_each(layers, [&result, aAntigenNo, num_sera = number_of_sera()](const rjson::value& layer, size_t layer_no) {
        if (const auto& for_ag = layer[aAntigenNo]; !for_ag.empty()) {
            for (size_t serum_no = 0; serum_no < num_sera; ++serum_no) {
                if (const auto& titer = for_ag[serum_no]; !titer.is_null()) {
                    result.push_back(layer_no);
                    break;
                }
            }
        }
    });
    return result;

} // ae::chart::v2::RjsonTiters::layers_with_antigen

// ----------------------------------------------------------------------

std::vector<size_t> ae::chart::v2::RjsonTiters::layers_with_serum(const rjson::value& layers, size_t aSerumNo) const
{
    if (layers.empty())
        throw data_not_available("no layers");
    std::vector<size_t> result;
    rjson::for_each(layers, [&result, aSerumNo, num_antigens = number_of_antigens()](const rjson::value& layer, size_t layer_no) {
        for (size_t antigen_no = 0; antigen_no < num_antigens; ++antigen_no) {
            if (const auto& for_ag = layer[antigen_no]; !for_ag.empty()) {
                if (const auto& titer = for_ag[aSerumNo]; !titer.is_null()) {
                    result.push_back(layer_no);
                    break;
                }
            }
        }
    });
    return result;

} // ae::chart::v2::RjsonTiters::layers_with_antigen

// ----------------------------------------------------------------------

size_t ae::chart::v2::RjsonTiters::number_of_non_dont_cares() const
{
    size_t result = 0;
    if (const auto& list = data_[keys_.list]; !list.is_null()) {
        rjson::for_each(list, [&result](const rjson::value& row) {
            rjson::for_each(row, [&result](const rjson::value& titer) {
                if (!Titer(titer.to<std::string_view>()).is_dont_care())
                    ++result;
            });
        });
    }
    else {
        rjson::for_each(data_[keys_.dict], [&result](const rjson::value& row) { result += row.size(); });
    }
    return result;

} // ae::chart::v2::RjsonTiters::number_of_non_dont_cares

// ----------------------------------------------------------------------

size_t ae::chart::v2::RjsonTiters::titrations_for_antigen(size_t antigen_no) const
{
    size_t result = 0;
    if (const auto& list = data_[keys_.list]; !list.is_null()) {
        const rjson::value& row = list[antigen_no];
        rjson::for_each(row, [&result](const rjson::value& titer) {
            if (!Titer(titer.to<std::string_view>()).is_dont_care())
                ++result;
        });
    }
    else {
        result += data_[keys_.dict][antigen_no].size();
    }
    return result;

} // ae::chart::v2::RjsonTiters::titrations_for_antigen

// ----------------------------------------------------------------------

size_t ae::chart::v2::RjsonTiters::titrations_for_serum(size_t serum_no) const
{
    size_t result = 0;
    if (const auto& list = data_[keys_.list]; !list.is_null()) {
        rjson::for_each(list, [&result, serum_no](const rjson::value& row) {
            if (!Titer(row[serum_no].to<std::string_view>()).is_dont_care())
                ++result;
        });
    }
    else {
        rjson::for_each(data_[keys_.dict], [&result, serum_no](const rjson::value& row) {
            if (const auto& titer = row[serum_no]; !titer.is_null() && !Titer{titer.to<std::string_view>()}.is_dont_care())
                ++result;
        });
    }
    return result;

} // ae::chart::v2::RjsonTiters::titrations_for_serum

// ----------------------------------------------------------------------

namespace
{
    class TiterGetterExistingBase : public ae::chart::v2::TiterIterator::TiterGetter
    {
      public:
        TiterGetterExistingBase(const rjson::value& titer_data) : titer_data_{titer_data} {}

        void last(ae::chart::v2::TiterIterator::Data& data) const override
        {
            data.antigen = titer_data_.size();
            data.serum = 0;
        }

      protected:
        bool valid(const ae::chart::v2::Titer& titer) const { return !titer.is_dont_care(); }
        size_t number_of_rows() const { return titer_data_.size(); }
        const rjson::value& row(ae::chart::v2::TiterIterator::Data& data) const { return titer_data_[data.antigen]; }
        const rjson::value& titer(ae::chart::v2::TiterIterator::Data& data) const { return titer_data_[data.antigen][data.serum]; }

      private:
        const rjson::value& titer_data_;
    };

    class TiterGetterExistingList : public TiterGetterExistingBase
    {
      public:
        using TiterGetterExistingBase::TiterGetterExistingBase;

        void first(ae::chart::v2::TiterIterator::Data& data) const override
        {
            data.antigen = 0;
            data.serum = 0;
            data.titer = ae::chart::v2::Titer{titer(data).to<std::string_view>()};
            if (!valid(data.titer))
                next(data);
        }

        void next(ae::chart::v2::TiterIterator::Data& data) const override
        {
            while (data.antigen < number_of_rows()) {
                ++data.serum;
                if (data.serum == row(data).size()) {
                    ++data.antigen;
                    data.serum = 0;
                }
                if (data.antigen < number_of_rows()) {
                    data.titer = ae::chart::v2::Titer{titer(data).to<std::string_view>()};
                    if (valid(data.titer))
                        break;
                }
                else
                    data.titer = ae::chart::v2::Titer{};
            }
        }
    };

    class TiterGetterExistingDict : public TiterGetterExistingBase
    {
      public:
        using TiterGetterExistingBase::TiterGetterExistingBase;

        void first(ae::chart::v2::TiterIterator::Data& data) const override
        {
            for (data.antigen = 0; data.antigen < number_of_rows() && row(data).empty(); ++data.antigen)
                ;
            if (data.antigen < number_of_rows())
                populate_sera(data);
        }

        void next(ae::chart::v2::TiterIterator::Data& data) const override
        {
            if (data.antigen >= number_of_rows()) {
                // last antigen dict was empty, there are not titers for the last antigen in the layer, perhaps after subsetting and removing sera
                data.serum = 0;
                data.titer = ae::chart::v2::Titer{};
                return;
            }
            if (serum_ == sera_.end())
                throw std::runtime_error{fmt::format("internal error in TiterGetterExistingDict::next @@ {}:{}", __FILE__, __LINE__)};
            ++serum_;
            if (serum_ == sera_.end()) {
                for (++data.antigen; data.antigen < number_of_rows() && row(data).empty(); ++data.antigen)
                    ;
                if (data.antigen < number_of_rows()) {
                    populate_sera(data);
                }
                else {
                    data.serum = 0;
                    data.titer = ae::chart::v2::Titer{};
                }
            }
            else {
                data.serum = *serum_;
                data.titer = ae::chart::v2::Titer{titer(data).to<std::string_view>()};
            }
        }

      private:
        mutable std::vector<size_t> sera_;
        mutable std::vector<size_t>::const_iterator serum_;

        void populate_sera(ae::chart::v2::TiterIterator::Data& data) const
        {
            rjson::transform(row(data), sera_, [](const rjson::object::value_type& kv) -> size_t { return std::stoul(kv.first); });
            std::sort(sera_.begin(), sera_.end());
            serum_ = sera_.begin();
            data.serum = *serum_;
            data.titer = ae::chart::v2::Titer{titer(data).to<std::string_view>()};
        }
    };
} // namespace

// ----------------------------------------------------------------------

ae::chart::v2::TiterIteratorMaker ae::chart::v2::RjsonTiters::titers_existing() const
{
    if (const auto& list = data_[keys_.list]; !list.is_null())
        return ae::chart::v2::TiterIteratorMaker(std::make_shared<TiterGetterExistingList>(list));
    else
        return ae::chart::v2::TiterIteratorMaker(std::make_shared<TiterGetterExistingDict>(data_[keys_.dict]));

} // ae::chart::v2::RjsonTiters::titers_existing

// ----------------------------------------------------------------------

ae::chart::v2::TiterIteratorMaker ae::chart::v2::RjsonTiters::titers_existing_from_layer(size_t layer_no) const
{
    const auto& data = data_[keys_.layers][layer_no];
    if (data.empty())
        throw invalid_data{AD_FORMAT("titers_existing_from_layer: empty list of titers, expected either list of lists or list of objects")};
    if (data[0].is_object())
        return ae::chart::v2::TiterIteratorMaker(std::make_shared<TiterGetterExistingDict>(data));
    else if (data[0].is_array())
        return ae::chart::v2::TiterIteratorMaker(std::make_shared<TiterGetterExistingList>(data));
    else
        throw invalid_data{AD_FORMAT("titers_existing_from_layer: invalid titers per antigen value ({}), expected either list of lists or list of objects", data[0])};

} // ae::chart::v2::RjsonTiters::titers_existing

// ----------------------------------------------------------------------

ae::chart::v2::rjson_import::Layout::Layout(const rjson::value& aData) : ae::chart::v2::Layout(aData.size(), rjson_import::number_of_dimensions(aData))
{
    auto coord = Vec::begin();
    rjson::for_each(aData, [&coord, num_dim = number_of_dimensions()](const rjson::value& point) {
        if (point.size() == *num_dim)
            rjson::transform(point, coord, [](const rjson::value& coordinate) -> double { return coordinate.to<double>(); });
        else if (!point.empty())
            throw invalid_data{AD_FORMAT("rjson_import::Layout: point has invalid number of coordinates: {}, expected 0 or {}", point.size(), num_dim)};
        coord += static_cast<decltype(coord)::difference_type>(*num_dim);
    });

} // ae::chart::v2::rjson_import::Layout::Layout

// ----------------------------------------------------------------------

static void update_list(const rjson::value& data, ae::chart::v2::TableDistances& table_distances, const ae::chart::v2::ColumnBases& column_bases, const ae::chart::v2::StressParameters& parameters, size_t number_of_points)
{
    const auto logged_adjusts = parameters.avidity_adjusts.logged(number_of_points);
    for (auto p1 : range_from_0_to(data.size())) {
        if (!parameters.disconnected.contains(p1)) {
            const auto& row = data[p1];
            for (auto serum_no : range_from_0_to(row.size())) {
                const auto p2 = serum_no + data.size();
                if (!parameters.disconnected.contains(p2)) {
                    table_distances.update(ae::chart::v2::Titer{row[serum_no].to<std::string_view>()}, p1, p2, column_bases.column_basis(serum_no), logged_adjusts[p1] + logged_adjusts[p2], parameters.mult);
                }
            }
        }
    }

} // update_list

static void update_dict(const rjson::value& data, ae::chart::v2::TableDistances& table_distances, const ae::chart::v2::ColumnBases& column_bases, const ae::chart::v2::StressParameters& parameters, size_t number_of_points)
{
    const auto logged_adjusts = parameters.avidity_adjusts.logged(number_of_points);
    for (auto p1 : range_from_0_to(data.size())) {
        if (!parameters.disconnected.contains(p1)) {
            rjson::for_each(data[p1], [num_antigens=data.size(),p1,&parameters,&table_distances,&column_bases,&logged_adjusts](std::string_view field_name, const rjson::value& field_value) {
                const auto serum_no = ae::from_chars<size_t>(field_name);
                const auto p2 = serum_no + num_antigens;
                if (!parameters.disconnected.contains(p2))
                    table_distances.update(ae::chart::v2::Titer{field_value.to<std::string_view>()}, p1, p2, column_bases.column_basis(serum_no), logged_adjusts[p1] + logged_adjusts[p2], parameters.mult);
            });
        }
    }

} // update_dict

void ae::chart::v2::rjson_import::update(const rjson::value& data, std::string_view list_key, const std::string& dict_key, TableDistances& table_distances, const ColumnBases& column_bases, const StressParameters& parameters, size_t number_of_points)
{
    table_distances.dodgy_is_regular(parameters.dodgy_titer_is_regular);
    if (const auto& list = data[list_key]; !list.is_null())
        ::update_list(list, table_distances, column_bases, parameters, number_of_points);
    else
        ::update_dict(data[dict_key], table_distances, column_bases, parameters, number_of_points);

} // ae::chart::v2::rjson_import::update

// ----------------------------------------------------------------------

ae::chart::v2::DisconnectedPoints ae::chart::v2::RjsonProjection::disconnected() const
{
    auto result = make_disconnected();
    if (result->empty()) {
          // infer disconnected from empty rows in layout
        auto lt = layout();
        for (size_t p_no = 0; p_no < lt->number_of_points(); ++p_no) {
            if (!lt->point_has_coordinates(p_no))
                result.insert(p_no);
        }
    }
    return result;

} // ae::chart::v2::RjsonProjection::disconnected

// ----------------------------------------------------------------------
