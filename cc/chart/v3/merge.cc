#include "chart/v3/merge.hh"
#include "chart/v3/chart.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    static void merge_info(Chart& merge, const Chart& chart1, const Chart& chart2);
    static Titers::titer_merge_report merge_titers(Chart& merge, const Chart& chart1, const Chart& chart2, const merge_data_t& merge_data);
    static void merge_legacy_plot_spec(Chart& merge, const Chart& chart1, const Chart& chart2, const merge_data_t& merge_data);

    template <typename AgSrs> static void merge_antigens_sera(AgSrs& merge, const AgSrs& source, const merge_data_t::index_mapping_t<typename AgSrs::index_t>& to_target, bool always_replace)
    {
        for (const auto no : source.size()) {
            if (const auto entry = to_target.find(no); entry != to_target.end()) {
                if (!always_replace && entry->second.common)
                    merge[entry->second.index].update_with(source[no]);
                else
                    merge[entry->second.index] = source[no];
                // AD_DEBUG("merge_antigens_sera {} {} <- {} {}", entry->second.index, merge.at(entry->second.index).full_name(), no, source.at(no)->full_name());
                if constexpr (std::is_same_v<AgSrs, Sera>)
                    merge[entry->second.index].homologous_antigens().get().clear();
            }
        }
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

std::pair<std::shared_ptr<ae::chart::v3::Chart>, ae::chart::v3::merge_data_t> ae::chart::v3::merge(std::shared_ptr<Chart> chart1, std::shared_ptr<Chart> chart2, const merge_settings_t& settings)
{
    try {
        chart1->throw_if_duplicates();
        chart2->throw_if_duplicates();

        merge_data_t merge_data{chart1, chart2, settings, common_antigens_sera_t{*chart1, *chart2, settings.match_level}};

        auto merged = std::make_shared<Chart>(); // merge_data.number_of_antigens_in_merge(), merge_data.number_of_sera_in_merge());
        merge_info(*merged, *chart1, *chart2);
        merged->antigens().resize(merge_data.number_of_antigens_in_merge());
        merged->sera().resize(merge_data.number_of_sera_in_merge());
        merge_antigens_sera(merged->antigens(), chart1->antigens(), merge_data.antigens_primary_target(), true);
        merge_antigens_sera(merged->antigens(), chart2->antigens(), merge_data.antigens_secondary_target(), false);
        merge_antigens_sera(merged->sera(), chart1->sera(), merge_data.sera_primary_target(), true);
        merge_antigens_sera(merged->sera(), chart2->sera(), merge_data.sera_secondary_target(), false);
        merged->throw_if_duplicates();
        merge_data.titer_report(merge_titers(*merged, *chart1, *chart2, merge_data));
    // merge_projections(*result, chart1, chart2, merge_data);
        // merge_semantic_plot_spec(*merged, *chart1, *chart2, merge_data);
        merge_legacy_plot_spec(*merged, *chart1, *chart2, merge_data);

        AD_WARNING("ae::chart::v3::merge is incomplete");
        return {std::move(merged), std::move(merge_data)};
    }
    catch (std::exception& err) {
        throw merge_error{err.what()};
    }

    // ----------------------------------------------------------------------

    // const auto relax_type4_type5 = [](const MergeReport& merge_data, ChartModify& chart) {
    //     UnmovablePoints unmovable_points;
    //     // set unmovable for all points of chart1 including common ones
    //     for (const auto& [index1, index_merge_common] : merge_data.antigens_primary_target)
    //         unmovable_points.insert(index_merge_common.index);
    //     for (const auto& [index1, index_merge_common] : merge_data.sera_primary_target)
    //         unmovable_points.insert(index_merge_common.index + chart.number_of_antigens());
    //     auto projection = chart.projection_modify(0);
    //     projection->set_unmovable(unmovable_points);
    //     projection->relax(optimization_options{});
    // };

    // // --------------------------------------------------

    // auto& result_antigens = result->antigens_modify();
    // auto& result_sera = result->sera_modify();

    // merge_plot_spec(*result, chart1, chart2, merge_data);

    // if (chart1.number_of_projections()) {
    //     switch (settings.projection_merge) {
    //         case projection_merge_t::type1:
    //             break; // no projections in the merge
    //         case projection_merge_t::type2:
    //             merge_projections_type2(*result, chart1, chart2, merge_data);
    //             break;
    //         case projection_merge_t::type3:
    //             if (chart2.number_of_projections() == 0)
    //                 throw merge_error{"cannot perform type3 merge: secondary chart has no projections"};
    //             merge_projections_type3(*result, chart1, chart2, merge_data);
    //             break;
    //         case projection_merge_t::type4:
    //             if (chart2.number_of_projections() == 0)
    //                 throw merge_error{"cannot perform type4 merge: secondary chart has no projections"};
    //             merge_projections_type3(*result, chart1, chart2, merge_data);
    //             // fmt::print(stderr, "DEBUG: merge type4 before relax stress {}\n", result->projection(0)->stress());
    //             relax_type4_type5(merge_data, *result);
    //             // fmt::print(stderr, "DEBUG: merge type4 after relax stress {}\n", result->projection(0)->stress());
    //             break;
    //         case projection_merge_t::type5:
    //             if (chart2.number_of_projections() == 0)
    //                 throw merge_error{"cannot perform type5 merge: secondary chart has no projections"};
    //             merge_projections_type5(*result, chart1, chart2, merge_data);
    //             // fmt::print(stderr, "DEBUG: merge type5 before relax stress {}\n", result->projection(0)->stress());
    //             relax_type4_type5(merge_data, *result);
    //             // fmt::print(stderr, "DEBUG: merge type5 after relax stress {}\n", result->projection(0)->stress());
    //             break;
    //     }
    // }

    // return {std::move(result), std::move(merge_data)};

} // ae::chart::v3::merge

// ----------------------------------------------------------------------

void ae::chart::v3::merge_data_t::build(const merge_settings_t& settings)
{
    // antigens
    {
        const auto& src1 = chart1_->antigens();
        for (const auto no1 : src1.size()) {
            if (settings.remove_distinct_ == remove_distinct::no || src1[no1].annotations().distinct()) {
                antigens_primary_target_.insert_or_assign(no1, target_index_common_t<antigen_index>{target_antigens_, common_.antigen_secondary_by_primary(no1).has_value()});
                ++target_antigens_;
            }
        }
        auto src2 = chart2_->antigens();
        const auto secondary_antigen_indexes = secondary_antigens_to_merge(settings);
        for (const auto no2 : secondary_antigen_indexes) {
            if (settings.remove_distinct_ == remove_distinct::no || !src2[no2].annotations().distinct()) {
                if (const auto no1 = common_.antigen_primary_by_secondary(no2); no1)
                    antigens_secondary_target_.insert_or_assign(no2, antigens_primary_target_.find(*no1)->second);
                else {
                    antigens_secondary_target_.insert_or_assign(no2, target_index_common_t<antigen_index>{target_antigens_, false});
                    ++target_antigens_;
                }
            }
        }
    }

    // sera
    {
        auto src1 = chart1_->sera();
        for (const auto no1 : src1.size()) {
            sera_primary_target_.insert_or_assign(no1, target_index_common_t<serum_index>{target_sera_, common_.serum_secondary_by_primary(no1).has_value()});
            ++target_sera_;
        }
        auto src2 = chart2_->sera();
        for (const auto no2 : src2.size()) {
            if (const auto no1 = common_.serum_primary_by_secondary(no2); no1)
                sera_secondary_target_.insert_or_assign(no2, sera_primary_target_.find(*no1)->second);
            else {
                sera_secondary_target_.insert_or_assign(no2, target_index_common_t<serum_index>{target_sera_, false});
                ++target_sera_;
            }
        }
    }

    // std::cerr << "DEBUG: antigens_primary_target " << antigens_primary_target << '\n';
    // std::cerr << "DEBUG: antigens_secondary_target" << antigens_secondary_target << '\n';
    // AD_DEBUG("sera_primary_target {}", sera_primary_target);
    // AD_DEBUG("sera_secondary_target {}", sera_secondary_target);

} // ae::chart::v3::merge_data_t::build

// ----------------------------------------------------------------------

ae::antigen_indexes ae::chart::v3::merge_data_t::secondary_antigens_to_merge(const merge_settings_t& settings) const
{
    auto indexes = index_range(chart2_->antigens().size());
    if (settings.combine_cheating_assays_ == combine_cheating_assays::yes) {
        // expected: primary chart is single or multi layered, secondary chart is single layered
        const auto& secondary_titers = chart2_->titers();
        if (secondary_titers.number_of_layers() > layer_index{1})
            AD_WARNING("[chart merge and combine_cheating_assays]: secondary chart is multilayered, result can be unexpected");

        bool all_secondary_in_primary{true}; // otherwise no cheating assay
        bool titers_same{true};              // otherwise no cheating assay

        const auto primary_by_secondary = [&all_secondary_in_primary]<typename Indexes>(const Indexes& indexes2, auto&& func) {
            Indexes result(indexes2.size());
            std::transform(indexes2.begin(), indexes2.end(), result.begin(),
                           [&all_secondary_in_primary, &func](auto no2) -> decltype(no2) {
                                                if (const auto no1 = func(no2); no1.has_value()) {
                                                    return *no1;
                                                }
                                                else {
                                                    all_secondary_in_primary = false;
                                                    return decltype(no2){invalid_index};
                                                }
                           });
            return result;
        };

        const auto antigens2_indexes = chart2_->reference();
        const auto sera2_indexes = index_range(chart2_->sera().size());
        const auto antigen_indexes1 = primary_by_secondary(antigens2_indexes, [this](auto no2) { return common_.antigen_primary_by_secondary(no2); });
        const auto serum_indexes1 = primary_by_secondary(sera2_indexes, [this](auto no2) { return common_.serum_primary_by_secondary(no2); });
        if (all_secondary_in_primary) {
            // check titers
            const auto are_titers_same = [&](auto&& primary_titer) -> bool {
                for (const auto antigen_index_no : antigen_index{antigens2_indexes.size()}) {
                    for (const auto serum_index_no : serum_index{sera2_indexes.size()}) {
                        if (secondary_titers.titer(antigens2_indexes[*antigen_index_no], sera2_indexes[*serum_index_no]) !=
                            primary_titer(antigen_indexes1[*antigen_index_no], serum_indexes1[*serum_index_no]))
                            return false;
                    }
                }
                return true;
            };

            const auto& primary_titers = chart1_->titers();
            if (primary_titers.number_of_layers() == layer_index{0}) {
                titers_same = are_titers_same([&primary_titers](antigen_index ag, serum_index sr) { return primary_titers.titer(ag, sr); });
            }
            else {
                titers_same = false;
                // if in any of the layers titers are the same
                for (const auto layer_no : primary_titers.number_of_layers()) {
                    titers_same |= are_titers_same([&primary_titers, layer_no](antigen_index ag, serum_index sr) { return primary_titers.titer_of_layer(layer_no, ag, sr); });
                    if (titers_same)
                        break;
                }
            }
        }

        if (all_secondary_in_primary && titers_same) {
            if (chart2_->antigens().size() == antigen_index{antigens2_indexes.size()}) {
                AD_ERROR("cheating assay ({}) and chart has no test antigens, remove table or disable cheating assay handling", chart2_->name());
                throw merge_error{"cheating assay and chart has no test antigens"};
            }
            auto test_indexes = index_range(chart2_->antigens().size());
            for (const auto ind : antigens2_indexes)
                test_indexes.remove(ind);
            AD_INFO("cheating assay ({}) will be combined, no reference titers will be in the new layer, test antigens: {}", chart2_->name(), test_indexes);
            return test_indexes;
        }
        else {
            // if (!all_secondary_in_primary)
            //     AD_INFO("[chart merge and combine_cheating_assays]: not a cheating assay: not all secondary ref antigens/sera found in primary");
            // if (!titers_same)
            //     AD_INFO("[chart merge and combine_cheating_assays]: not a cheating assay: titers are different");
        }
    }

    return index_range(chart2_->antigens().size()); // no cheating assay or combining not requested

} // ae::chart::v3::merge_data_t::secondary_antigens_to_merge

// ----------------------------------------------------------------------

std::string ae::chart::v3::merge_data_t::titer_merge_report(const Chart& chart) const
{
    const auto max_field = std::max(static_cast<int>(std::max(chart.antigens().max_designation(), chart.info().max_source_name())), 20);

    fmt::memory_buffer output;
    fmt::format_to(std::back_inserter(output), "Acmacs merge table and diagnositics\n\n{}\n{}\n\n", chart.name(), "*table*" /*chart.show_table()*/);

    fmt::format_to(std::back_inserter(output), "                                   DIAGNOSTICS\n         (common titers, and how they merged, and the individual tables)\n\n");
    fmt::format_to(std::back_inserter(output), "{}\n", titer_merge_diagnostics(chart, index_range(chart.antigens().size()), index_range(chart.sera().size()), max_field));

    for (const auto layer_no : chart.titers().number_of_layers()) {
        if (layer_no < layer_index{chart.info().sources().size()})
            fmt::format_to(std::back_inserter(output), "{}\n", chart.info().sources()[*layer_no].name_or_date());
        else
            fmt::format_to(std::back_inserter(output), "layer {}\n", layer_no);
        // fmt::format_to(std::back_inserter(output), "{}\n\n", chart.show_table(layer_no));
    }

    fmt::format_to(std::back_inserter(output), "    Table merge subset showing only rows and columns that have merged values\n        (same as first diagnostic output, but subsetted for changes only)\n");
    const auto [antigens, sera] = chart.titers().antigens_sera_in_multiple_layers();
    fmt::format_to(std::back_inserter(output), "{}\n", titer_merge_diagnostics(chart, antigens, sera, max_field));

    return fmt::to_string(output);

} // ae::chart::v3::merge_data_t::titer_merge_report

// ----------------------------------------------------------------------

std::string ae::chart::v3::merge_data_t::titer_merge_report_common_only(const Chart& chart) const
{
    return "> ae::chart::v3::merge_data_t::titer_merge_report_common_only not implemented";

    const auto max_field = std::max(static_cast<int>(std::max(chart.antigens().max_designation(), chart.info().max_source_name())), 20);

    fmt::memory_buffer output;
    fmt::format_to(std::back_inserter(output), "Acmacs merge diagnositics for common antigens and sera\n\n{}\n\n", chart.name());

    if (!common_.antigens().empty() && !common_.sera().empty()) {
        fmt::format_to(std::back_inserter(output), "                                   DIAGNOSTICS\n         (common titers, and how they merged, and the individual tables)\n\n");
        fmt::format_to(std::back_inserter(output), "{}\n", titer_merge_diagnostics(chart, common_.primary_antigens(), common_.primary_sera(), max_field));

        // for (auto layer_no : acmacs::range(chart.titers()->number_of_layers())) {
        //     if (layer_no < chart.info()->number_of_sources())
        //         fmt::format_to(std::back_inserter(output), "{}\n", chart.info()->source(layer_no)->name_non_empty());
        //     else
        //         fmt::format_to(std::back_inserter(output), "layer {}\n", layer_no);
        //     fmt::format_to(std::back_inserter(output), "{}\n\n", chart.show_table(layer_no));
        // }

        // fmt::format_to(std::back_inserter(output), "    Table merge subset showing only rows and columns that have merged values\n        (same as first diagnostic output, but subsetted for changes only)\n");
        // const auto [antigens, sera] = chart.titers()->antigens_sera_in_multiple_layers();
        // fmt::format_to(std::back_inserter(output), "{}\n", titer_merge_diagnostics(chart, antigens, sera, max_field));
    }
    else
        fmt::format_to(std::back_inserter(output), ">> WARNING: common only report cannot be done: common antigens: {} common sera: {}", common_.antigens(), common_.sera());

    return fmt::to_string(output);

} // ae::chart::v3::merge_data_t::titer_merge_report_common_only

// ----------------------------------------------------------------------

std::string ae::chart::v3::merge_data_t::titer_merge_diagnostics(const Chart& chart, const antigen_indexes& antigens, const serum_indexes& sera, int max_field_size) const
{
    return "> ae::chart::v3::merge_data_t::titer_merge_diagnostics not implemented";

    // // auto sr_label = [](size_t sr_no) -> char { return static_cast<char>('A' + sr_no); };
    // auto sr_label = [](size_t sr_no) -> size_t { return sr_no + 1; };
    // auto ags = chart.antigens();
    // auto srs = chart.sera();
    // auto tt = chart.titers();

    // fmt::memory_buffer output;
    // fmt::format_to(std::back_inserter(output), "{:{}s}", "", max_field_size);
    // for (auto sr_no : sera)
    //     fmt::format_to(std::back_inserter(output), "{:>7d}", sr_label(sr_no));
    // fmt::format_to(std::back_inserter(output), "\n{:{}s}", "", max_field_size + 2);
    // for (auto sr_no : sera)
    //     fmt::format_to(std::back_inserter(output), "{:>7s}", srs->at(sr_no)->format("{location_abbreviated}/{year2}"));
    // fmt::format_to(std::back_inserter(output), "\n");

    // for (auto ag_no : antigens) {
    //     auto antigen = ags->at(ag_no);
    //     fmt::format_to(std::back_inserter(output), "{}\n", antigen->name_full());
    //     for (auto layer_no : acmacs::range(tt->number_of_layers())) {
    //         if (layer_no < chart.info()->number_of_sources())
    //             fmt::format_to(std::back_inserter(output), "{:<{}s}", chart.info()->source(layer_no)->name_non_empty(), max_field_size + 2);
    //         else
    //             fmt::format_to(std::back_inserter(output), "{:<{}s}", fmt::format("layer {}", layer_no), max_field_size + 2);
    //         for (auto sr_no : sera) {
    //             auto titer = tt->titer_of_layer(layer_no, ag_no, sr_no);
    //             fmt::format_to(std::back_inserter(output), "{:>7s}", (titer.is_dont_care() ? "" : *titer));
    //         }
    //         fmt::format_to(std::back_inserter(output), "\n");
    //     }
    //     fmt::format_to(std::back_inserter(output), "{:<{}s}", "Merge", max_field_size + 2);
    //     for (auto sr_no : sera)
    //         fmt::format_to(std::back_inserter(output), "{:>7s}", *tt->titer(ag_no, sr_no));
    //     fmt::format_to(std::back_inserter(output), "\n");

    //     fmt::format_to(std::back_inserter(output), "{:<{}s}", "Report (see below)", max_field_size + 2);
    //     for (auto sr_no : sera) {
    //         if (const auto found = std::find_if(titer_report->begin(), titer_report->end(), [ag_no=ag_no,sr_no](const auto& entry) { return entry.antigen == ag_no && entry.serum == sr_no; }); found != titer_report->end())
    //             fmt::format_to(std::back_inserter(output), "{:>7s}", TitersModify::titer_merge_report_brief(found->report));
    //         else
    //             fmt::format_to(std::back_inserter(output), "       ");
    //     }
    //     fmt::format_to(std::back_inserter(output), "\n\n");
    // }

    // fmt::format_to(std::back_inserter(output), "{}\n", TitersModify::titer_merge_report_description());

    // return fmt::to_string(output);


} // ae::chart::v3::merge_data_t::titer_merge_diagnostics

// ----------------------------------------------------------------------

void ae::chart::v3::merge_info(Chart& merge, const Chart& chart1, const Chart& chart2)
{
    merge.info().virus(chart1.info().virus());
    if (chart1.info().sources().empty())
        merge.info().sources().push_back(chart1.info());
    else
        std::copy(chart1.info().sources().begin(), chart1.info().sources().end(), std::back_inserter(merge.info().sources()));
    if (chart2.info().sources().empty())
        merge.info().sources().push_back(chart2.info());
    else
        std::copy(chart2.info().sources().begin(), chart2.info().sources().end(), std::back_inserter(merge.info().sources()));

} // ae::chart::v3::merge_info

// ----------------------------------------------------------------------

ae::chart::v3::Titers::titer_merge_report ae::chart::v3::merge_titers(Chart& merge, const Chart& chart1, const Chart& chart2, const merge_data_t& merge_data)
{
    auto &titers = merge.titers(), titers1 = chart1.titers(), titers2 = chart2.titers();
    auto layers1 = titers1.number_of_layers(), layers2 = titers2.number_of_layers();
    titers.create_layers((layers1 > layer_index{1} ? layers1 : layer_index{1}) + (layers2 > layer_index{1} ? layers2 : layer_index{1}), merge.antigens().size());

    layer_index target_layer_no{0};
    const auto copy_layers = [&target_layer_no, &titers](layer_index source_layers, const Titers& source_titers, const auto& antigen_target, const auto& serum_target) {
        const auto assign = [&target_layer_no, &titers](antigen_index ag_no, serum_index sr_no, const Titer& titer) { titers.set_titer_of_layer(target_layer_no, ag_no, sr_no, titer); };
        auto copy_titer = [assign, &antigen_target, &serum_target](const auto& titer_iterator_gen) {
            for (auto titer_ref : titer_iterator_gen) {
                const auto ag_no = antigen_target.find(titer_ref.antigen);
                const auto sr_no = serum_target.find(titer_ref.serum);
                if (ag_no != antigen_target.end() && sr_no != serum_target.end())
                    assign(ag_no->second.index, sr_no->second.index, titer_ref.titer);
            }
        };

        if (source_layers > layer_index{1}) {
            for (const auto source_layer_no : source_layers) {
                copy_titer(source_titers.titers_existing_from_layer(source_layer_no));
                ++target_layer_no;
            }
        }
        else {
            copy_titer(source_titers.titers_existing());
            ++target_layer_no;
        }
    };
    copy_layers(layers1, titers1, merge_data.antigens_primary_target(), merge_data.sera_primary_target());
    copy_layers(layers2, titers2, merge_data.antigens_secondary_target(), merge_data.sera_secondary_target());
    return titers.set_from_layers(merge);

} // ae::chart::v3::merge_titers

// ----------------------------------------------------------------------

void ae::chart::v3::merge_legacy_plot_spec(Chart& merge, const Chart& chart1, const Chart& /*chart2*/, const merge_data_t& merge_data)
{
    // copy chart1 plot spec, ignore chart2 plot spec

    auto& merge_plot_spec = merge.legacy_plot_spec();
    merge_plot_spec = chart1.legacy_plot_spec();

    // adjust serum indexes in the merged plot style index
    const point_index index_adjust{*merge.antigens().size() - *chart1.antigens().size()};
    for (auto& p_no : merge_plot_spec.style_for_point()) {
        if (*p_no >= *chart1.antigens().size())
            p_no = p_no + index_adjust;
    }

    // reset drawing order
    merge_plot_spec.drawing_order().get().clear();


    // const auto& plot_spec2 = chart2.legacy_plot_spec();
    // for (const auto ag_no : chart1.antigens().size())
    //     merge_plot_spec.modify(ag_no, plot_spec1->style(ag_no));
    // for (const auto sr_no : chart1.sera().size())
    //     merge_plot_spec.modify_serum(sr_no, plot_spec1->style(sr_no + chart1.number_of_antigens()));
    // for (const auto ag_no : chart2.antigens().size())
    //     if (auto found = report.antigens_secondary_target.find(ag_no); found != report.antigens_secondary_target.end() && !found->second.common)
    //         merge_plot_spec.modify(found->second.index, plot_spec2->style(ag_no));
    // }
    // for (const auto sr_no : chart2.sera().size())
    //     if (auto found = report.sera_secondary_target.find(sr_no); found != report.sera_secondary_target.end() && !found->second.common)
    //         merge_plot_spec.modify_serum(found->second.index, plot_spec2->style(sr_no + chart2.number_of_antigens()));
    // }

      // drawing order
      // auto& drawing_order = merge_plot_spec.drawing_order_modify();

} // ae::chart::v3::merge_plot_spec

// ----------------------------------------------------------------------
