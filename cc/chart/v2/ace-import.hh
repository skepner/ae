#pragma once

#include "chart/v2/chart.hh"
#include "chart/v2/verify.hh"
#include "chart/v2/rjson-import.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v2
{
    namespace ace
    {
        using name_index_t = std::map<std::string_view, std::vector<size_t>>;
    }

    class AceChart : public Chart
    {
      public:
        AceChart(rjson::value&& aSrc) : data_{std::move(aSrc)} {}

        InfoP info() const override;
        AntigensP antigens() const override;
        SeraP sera() const override;
        TitersP titers() const override;
        ColumnBasesP forced_column_bases(MinimumColumnBasis aMinimumColumnBasis) const override;
        ProjectionsP projections() const override;
        PlotSpecP plot_spec() const override;
        bool is_merge() const override;
        bool has_sequences() const override;

        void verify_data(Verify aVerify) const;

          // to obtain extension fields (e.g. group_sets, gui data)
        const rjson::value& extension_field(std::string_view field_name) const override;
        const rjson::value& extension_fields() const override;

     private:
        rjson::value data_;
        mutable ace::name_index_t mAntigenNameIndex;
        mutable ProjectionsP projections_;

    }; // class AceChart

    bool is_ace(std::string_view aData);
    ChartP ace_import(std::string_view aData, Verify aVerify);

// ----------------------------------------------------------------------

    class AceInfo : public Info
    {
      public:
        AceInfo(const rjson::value& aData) : data_{aData} {}

        std::string name(Compute aCompute = Compute::No) const override;
        Virus       virus(Compute aCompute = Compute::No) const override { return Virus{make_field("v", "+", aCompute)}; }
        ae::virus::type_subtype_t   virus_type(Compute aCompute = Compute::Yes) const override { return ae::virus::type_subtype_t{make_field("V", "+", aCompute)}; }
        std::string subset(Compute aCompute = Compute::No) const override { return make_field("s", "+", aCompute); }
        Assay       assay(Compute aCompute = Compute::No) const override { return Assay{make_field("A", "+", aCompute)}; }
        Lab         lab(Compute aCompute = Compute::No, FixLab fix = FixLab::yes) const override { return Lab{fix_lab_name(Lab{make_field("l", "+", aCompute)}, fix)}; }
        RbcSpecies  rbc_species(Compute aCompute = Compute::No) const override { return RbcSpecies{make_field("r", "+", aCompute)}; }
        TableDate   date(Compute aCompute = Compute::No) const override;
        size_t number_of_sources() const override { return data_["S"].size(); }
        InfoP source(size_t aSourceNo) const override { return std::make_shared<AceInfo>(data_["S"][aSourceNo]); }

     private:
        const rjson::value& data_;

        std::string make_field(const char* aField, std::string_view aSeparator, Compute aCompute) const;

    }; // class AceInfo

// ----------------------------------------------------------------------

    class AceAntigen : public Antigen
    {
      public:
        AceAntigen(const rjson::value& aData) : data_{aData} {}

        ae::virus::Name name() const override { return ae::virus::Name{data_["N"].to<std::string_view>()}; }
        Date date() const override { return Date{data_["D"].get_or_default("")}; }
        ae::virus::Passage passage() const override { return ae::virus::Passage{data_["P"].get_or_default("")}; }
        BLineage lineage() const override;
        ae::virus::Reassortant reassortant() const override { return ae::virus::Reassortant{data_["R"].get_or_default("")}; }
        LabIds lab_ids() const override { return data_["l"]; }
        Clades clades() const override { return data_["c"]; }
        Annotations annotations() const override { const auto& rann = data_["a"]; Annotations ann(rann.size()); rjson::copy(rann, ann.begin()); return ann; }
        bool reference() const override { return data_["S"].get_or_default("").find("R") != std::string::npos; }
        bool sequenced() const override { return !data_["A"].empty(); }
        std::string sequence_aa() const override { return data_["A"].get_or_default(""); }
        std::string sequence_nuc() const override { return data_["B"].get_or_default(""); }

      private:
        const rjson::value& data_;

    }; // class AceAntigen

// ----------------------------------------------------------------------

    class AceSerum : public Serum
    {
      public:
        AceSerum(const rjson::value& aData) : data_{aData} {}

        ae::virus::Name name() const override { return ae::virus::Name{data_["N"].to<std::string_view>()}; }
        ae::virus::Passage passage() const override { return ae::virus::Passage{data_["P"].get_or_default("")}; }
        BLineage lineage() const override;
        ae::virus::Reassortant reassortant() const override { return ae::virus::Reassortant{data_["R"].get_or_default("")}; }
        Annotations annotations() const override { const auto& rann = data_["a"]; Annotations ann(rann.size()); rjson::copy(rann, ann.begin()); return ann; }
        Clades clades() const override { return data_["c"]; }
        SerumId serum_id() const override { return SerumId{data_["I"].get_or_default("")}; }
        SerumSpecies serum_species() const override { return SerumSpecies{data_["s"].get_or_default("")}; }
        PointIndexList homologous_antigens() const override { return data_["h"]; }
        void set_homologous(const std::vector<size_t>& ags, ae::debug) const override { const_cast<rjson::value&>(data_)["h"] = rjson::array(ags.begin(), ags.end()); }
        bool sequenced() const override { return !data_["A"].empty(); }
        std::string sequence_aa() const override { return data_["A"].get_or_default(""); }
        std::string sequence_nuc() const override { return data_["B"].get_or_default(""); }

     private:
        const rjson::value& data_;

    }; // class AceSerum

// ----------------------------------------------------------------------

    class AceAntigens : public Antigens
    {
     public:
        AceAntigens(const rjson::value& aData, ace::name_index_t& aAntigenNameIndex) : data_{aData}, mAntigenNameIndex{aAntigenNameIndex} {}

        size_t size() const override { return data_.size(); }
        AntigenP operator[](size_t aIndex) const override { return std::make_shared<AceAntigen>(data_[aIndex]); }
        std::optional<size_t> find_by_full_name(std::string_view aFullName) const override;

     private:
        const rjson::value& data_;
        ace::name_index_t& mAntigenNameIndex;

        void make_name_index() const;

    }; // class AceAntigens

// ----------------------------------------------------------------------

    class AceSera : public Sera
    {
      public:
        AceSera(const rjson::value& aData) : data_{aData} {}

        size_t size() const override { return data_.size(); }
        SerumP operator[](size_t aIndex) const override { return std::make_shared<AceSerum>(data_[aIndex]); }

     private:
        const rjson::value& data_;

    }; // class AceSera

// ----------------------------------------------------------------------

    class AceTiters : public RjsonTiters
    {
      public:
        AceTiters(const rjson::value& data, size_t number_of_antigens, size_t number_of_sera) : RjsonTiters(data, s_keys_, number_of_antigens, number_of_sera) {}

     private:
        static const Keys s_keys_;

    }; // class AceTiters

// ----------------------------------------------------------------------

    class AceColumnBases : public ColumnBases
    {
      public:
        AceColumnBases(const rjson::value& data) : data_{data} {}
        AceColumnBases(const rjson::value& data, MinimumColumnBasis minimum_column_basis) : data_{data}, minimum_column_basis_{minimum_column_basis} {}

        double column_basis(size_t aSerumNo) const override
        {
            try {
                return minimum_column_basis_.apply(data_[aSerumNo].to<double>());
            }
            catch (std::exception& err) {
                AD_ERROR("cannot read column bases (serum no: {}): {}\ndata: {}", aSerumNo, err, data_);
                throw;
            }
        }

        size_t size() const override { return data_.size(); }

      private:
        const rjson::value& data_;
        MinimumColumnBasis minimum_column_basis_;

    }; // class AceColumnBases

// ----------------------------------------------------------------------

    class AceProjection : public RjsonProjection
    {
      public:
        AceProjection(const Chart& chart, const rjson::value& aData) : RjsonProjection(chart, aData, s_keys_) {}
        AceProjection(const Chart& chart, const rjson::value& aData, size_t projection_no) : RjsonProjection(chart, aData, s_keys_, projection_no) {}

        MinimumColumnBasis minimum_column_basis() const override { return data()["m"].get_or_default("none"); }
        ColumnBasesP forced_column_bases() const override;
        ae::draw::v1::Transformation transformation() const override;
        enum dodgy_titer_is_regular dodgy_titer_is_regular() const override { return data()["d"].get_or_default(false) ? dodgy_titer_is_regular::yes : dodgy_titer_is_regular::no; }
        double stress_diff_to_stop() const override { return data()["d"].get_or_default(0.0); }
        UnmovablePoints unmovable() const override { return data()["U"]; }
        UnmovableInTheLastDimensionPoints unmovable_in_the_last_dimension() const override { return data()["u"]; }
        AvidityAdjusts avidity_adjusts() const override { return AvidityAdjusts{data()["f"]}; }

     protected:
        DisconnectedPoints make_disconnected() const override { return data()["D"]; }

     private:
        static const Keys s_keys_;

    }; // class AceProjections

// ----------------------------------------------------------------------

    class AceProjections : public Projections
    {
      public:
        AceProjections(const Chart& chart, const rjson::value& aData) : Projections(chart), data_{aData}, projections_(aData.size(), nullptr) {}

        bool empty() const override { return projections_.empty(); }
        size_t size() const override { return projections_.size(); }
        ProjectionP operator[](size_t aIndex) const override
            {
                if (!projections_[aIndex])
                    projections_[aIndex] = std::make_shared<AceProjection>(chart(), data_[aIndex], aIndex);
                return projections_[aIndex];
            }

     private:
        const rjson::value& data_;
        mutable std::vector<ProjectionP> projections_;

    }; // class AceProjections

// ----------------------------------------------------------------------

    class AcePlotSpec : public PlotSpec
    {
      public:
        AcePlotSpec(const rjson::value& aData, const AceChart& aChart) : data_{aData}, mChart{aChart} {}

        bool empty() const override { return data_.empty(); }
        DrawingOrder drawing_order() const override { return data_["d"]; }
        Color error_line_positive_color() const override;
        Color error_line_negative_color() const override;
        acmacs::PointStyle style(size_t aPointNo) const override;
        std::vector<acmacs::PointStyle> all_styles() const override;
        size_t number_of_points() const override;

     private:
        const rjson::value& data_;
        const AceChart& mChart;

        acmacs::PointStyle extract(const rjson::value& aSrc, size_t aPointNo, size_t aStyleNo) const;
        void label_style(acmacs::PointStyle& aStyle, const rjson::value& aData) const;

    }; // class AcePlotSpec

// ----------------------------------------------------------------------

} // namespace ae::chart::v2

// ----------------------------------------------------------------------
