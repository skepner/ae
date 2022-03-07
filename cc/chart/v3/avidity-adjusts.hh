#pragma once

#include "utils/named-vector.hh"
#include "utils/float.hh"
#include "utils/log.hh"
#include "chart/v3/index.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    using avidity_adjusts = ae::named_vector_t<double, struct avidity_adjusts_tag>;

    inline bool empty(const avidity_adjusts& aa)
    {
        return aa.get().empty() || std::all_of(aa.begin(), aa.end(), [](double val) -> bool { return float_equal(val, 1.0); });
    }

    inline avidity_adjusts avidity_adjusts_from_logged(const std::vector<double>& logged_adjusts)
    {
        avidity_adjusts aa(logged_adjusts.size(), 1.0);
        std::transform(std::begin(logged_adjusts), std::end(logged_adjusts), std::begin(aa), [](double logged) { return std::exp2(logged); });
        return aa;
    }

    inline std::vector<double> logged(const avidity_adjusts& aa)
    {
        std::vector<double> logged_adjusts(aa.size(), 0.0);
        if (!aa.empty())
            std::transform(aa.begin(), aa.end(), logged_adjusts.begin(), [](double adj) { return std::log2(adj); });
        return logged_adjusts;
    }

    inline void set_logged(avidity_adjusts& aa, antigen_index antigen_no, double logged_adjust)
    {
        if (aa.size() <= antigen_no.get())
            throw std::runtime_error{AD_FORMAT("set_logged: cannot set adjust for AG {} to {}: too few elements in the AvidityAdjusts list ({})", antigen_no, logged_adjust, aa.size())};
        aa[antigen_no.get()] = std::exp2(logged_adjust);
    }

    inline void resize(avidity_adjusts& aa, antigen_index number_of_points)
    {
        if (aa.size() == 0)
            aa.get().resize(number_of_points.get(), 1.0);
        else if (antigen_index{aa.size()} != number_of_points)
            throw std::runtime_error{AD_FORMAT("attempt to resize AvidityAdjusts from {} to {}", aa.size(), number_of_points)};
    }

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
