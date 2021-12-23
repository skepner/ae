#pragma once

#include "chart/v2/chart.hh"
#include "chart/v2/verify.hh"
#include "chart/v2/lispmds-token.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v2
{
    class LispmdsChart : public Chart
    {
      public:
        LispmdsChart(acmacs::lispmds::value&& aSrc) : mData{std::move(aSrc)} {}

        InfoP info() const override;
        AntigensP antigens() const override;
        SeraP sera() const override;
        TitersP titers() const override;
        ColumnBasesP forced_column_bases(MinimumColumnBasis aMinimumColumnBasis) const override;
        ProjectionsP projections() const override;
        PlotSpecP plot_spec() const override;
        bool is_merge() const override { return false; }
        bool has_sequences() const override { return false; }

        size_t number_of_antigens() const override;
        size_t number_of_sera() const override;

        void verify_data(Verify aVerify) const;

     private:
        acmacs::lispmds::value mData;
        mutable ProjectionsP projections_;

    }; // class Chart

    inline bool is_lispmds(std::string_view aData)
    {
        if (aData.size() < 100)
            return false;
        const auto start = aData.find("(MAKE-MASTER-MDS-WINDOW");
        if (start == std::string_view::npos)
            return false;
        const auto hi_in = aData.find("(HI-IN", start + 23);
          // "(TAB-IN" is not supported, structure is unclear
        if (hi_in == std::string_view::npos)
            return false;
        return true;
    }

    ChartP lispmds_import(std::string_view aData, Verify aVerify);

// ----------------------------------------------------------------------

    class LispmdsInfo : public Info
    {
      public:
        LispmdsInfo(const acmacs::lispmds::value& aData) : mData{aData} {}

        std::string name(Compute aCompute = Compute::No) const override;
        Virus       virus(Compute = Compute::No) const override { return {}; }
        ae::virus::type_subtype_t   virus_type(Compute = Compute::No) const override { return {}; }
        std::string subset(Compute = Compute::No) const override { return {}; }
        Assay       assay(Compute = Compute::No) const override { return {}; }
        Lab         lab(Compute = Compute::No, FixLab = FixLab::yes) const override { return {}; }
        RbcSpecies  rbc_species(Compute = Compute::No) const override { return{}; }
        TableDate   date(Compute = Compute::No) const override { return {}; }
        size_t number_of_sources() const override { return 0; }
        InfoP source(size_t) const override { return nullptr; }

     private:
        const acmacs::lispmds::value& mData;

    }; // class LispmdsInfo

// ----------------------------------------------------------------------

    class LispmdsAntigen : public Antigen
    {
      public:
        LispmdsAntigen(const acmacs::lispmds::value& aData, size_t aIndex) : mData{aData}, mIndex{aIndex} {}

        ae::virus::Name name() const override;
        Date date() const override { return {}; }
        ae::virus::Passage passage() const override;
        BLineage lineage() const override { return {}; }
        ae::virus::Reassortant reassortant() const override;
        LabIds lab_ids() const override { return {}; }
        Clades clades() const override { return {}; }
        Annotations annotations() const override;
        bool reference() const override;

     private:
        const acmacs::lispmds::value& mData;
        size_t mIndex;

    }; // class LispmdsAntigen

// ----------------------------------------------------------------------

    class LispmdsSerum : public Serum
    {
      public:
        LispmdsSerum(const acmacs::lispmds::value& aData, size_t aIndex) : mData{aData}, mIndex{aIndex} {}

        ae::virus::Name name() const override;
        ae::virus::Passage passage() const override { return {}; }
        BLineage lineage() const override { return {}; }
        ae::virus::Reassortant reassortant() const override;
        Annotations annotations() const override;
        Clades clades() const override { return {}; }
        SerumId serum_id() const override;
        SerumSpecies serum_species() const override { return {}; }
        PointIndexList homologous_antigens() const override { return {}; }

     private:
        const acmacs::lispmds::value& mData;
        size_t mIndex;

    }; // class LispmdsSerum

// ----------------------------------------------------------------------

    class LispmdsAntigens : public Antigens
    {
     public:
        LispmdsAntigens(const acmacs::lispmds::value& aData) : mData{aData} {}

        size_t size() const override;
        AntigenP operator[](size_t aIndex) const override;

     private:
        const acmacs::lispmds::value& mData;

    }; // class LispmdsAntigens

// ----------------------------------------------------------------------

    class LispmdsSera : public Sera
    {
      public:
        LispmdsSera(const acmacs::lispmds::value& aData) : mData{aData} {}

        size_t size() const override;
        SerumP operator[](size_t aIndex) const override;

     private:
        const acmacs::lispmds::value& mData;

    }; // class LispmdsSera

// ----------------------------------------------------------------------

    class LispmdsTiters : public Titers
    {
      public:
        LispmdsTiters(const acmacs::lispmds::value& aData) : mData{aData} {}

        Titer titer(size_t aAntigenNo, size_t aSerumNo) const override;
        Titer titer_of_layer(size_t, size_t, size_t) const override { throw data_not_available("lispmds importer does not support layers"); }
        std::vector<Titer> titers_for_layers(size_t /*aAntigenNo*/, size_t /*aSerumNo*/, include_dotcare = include_dotcare::no) const override { throw data_not_available("lispmds importer does not support layers"); }
        std::vector<size_t> layers_with_antigen(size_t /*aAntigenNo*/) const override { throw data_not_available("lispmds importer does not support layers"); }
        std::vector<size_t> layers_with_serum(size_t /*aSerumNo*/) const override { throw data_not_available("lispmds importer does not support layers"); }
        size_t number_of_layers() const override { return 0; }
        size_t number_of_antigens() const override;
        size_t number_of_sera() const override;
        size_t number_of_non_dont_cares() const override;
        size_t titrations_for_antigen(size_t antigen_no) const override;
        size_t titrations_for_serum(size_t serum_no) const override;

     private:
        const acmacs::lispmds::value& mData;

    }; // class LispmdsTiters

// ----------------------------------------------------------------------

    class LispmdsColumnBases : public ColumnBases
    {
      public:
        LispmdsColumnBases(const std::vector<double>& aData) : mData{aData} {}

        double column_basis(size_t aSerumNo) const override { return mData.at(aSerumNo); }
        size_t size() const override { return mData.size(); }

     private:
        std::vector<double> mData{};

    }; // class LispmdsColumnBases

// ----------------------------------------------------------------------

    class LispmdsProjection : public Projection
    {
      public:
        LispmdsProjection(const Chart& chart, const acmacs::lispmds::value& aData, size_t aIndex, size_t aNumberOfAntigens, size_t aNumberOfSera)
            : Projection(chart), mData{aData}, mNumberOfAntigens{aNumberOfAntigens}, mNumberOfSera{aNumberOfSera} { set_projection_no(aIndex); check(); }

        void check() const;
        std::optional<double> stored_stress() const override;
        std::shared_ptr<Layout> layout() const override;
        std::string comment() const override { return {}; }
        size_t number_of_points() const override { return mNumberOfAntigens + mNumberOfSera; }
        number_of_dimensions_t number_of_dimensions() const override;
        MinimumColumnBasis minimum_column_basis() const override;
        ColumnBasesP forced_column_bases() const override;
        ae::draw::v1::Transformation transformation() const override;
        enum dodgy_titer_is_regular dodgy_titer_is_regular() const override { return dodgy_titer_is_regular::no; }
        double stress_diff_to_stop() const override { return 0.0; }
        UnmovablePoints unmovable() const override;
        DisconnectedPoints disconnected() const override;
        UnmovableInTheLastDimensionPoints unmovable_in_the_last_dimension() const override { return {}; }
        AvidityAdjusts avidity_adjusts() const override;

     private:
        const acmacs::lispmds::value& mData;
        size_t mNumberOfAntigens{0}, mNumberOfSera{0};
        mutable std::shared_ptr<Layout> layout_{};

    }; // class LispmdsProjections

// ----------------------------------------------------------------------

    class LispmdsProjections : public Projections
    {
      public:
        LispmdsProjections(const Chart& chart, const acmacs::lispmds::value& aData) : Projections(chart), mData{aData}, projections_(size(), nullptr) {}

        bool empty() const override;
        size_t size() const override;
        ProjectionP operator[](size_t aIndex) const override;

     private:
        const acmacs::lispmds::value& mData;
        mutable std::vector<ProjectionP> projections_{};

    }; // class LispmdsProjections

// ----------------------------------------------------------------------

    class LispmdsPlotSpec : public PlotSpec
    {
      public:
        LispmdsPlotSpec(const acmacs::lispmds::value& aData) : mData{aData} {}

        bool empty() const override;
        DrawingOrder drawing_order() const override;
        Color error_line_positive_color() const override;
        Color error_line_negative_color() const override;
        acmacs::PointStyle style(size_t aPointNo) const override;
        std::vector<acmacs::PointStyle> all_styles() const override;
        size_t number_of_points() const override;

     private:
        const acmacs::lispmds::value& mData;

        void extract_style(acmacs::PointStyle& aTarget, size_t aPointNo) const;
        void extract_style(acmacs::PointStyle& aTarget, const acmacs::lispmds::list& aSource) const;

    }; // class LispmdsPlotSpec

// ----------------------------------------------------------------------

} // namespace ae::chart::v2

// ----------------------------------------------------------------------
