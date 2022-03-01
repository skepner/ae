#include "chart/v3/info.hh"

// ----------------------------------------------------------------------

std::string ae::chart::v3::Assay::hi_or_neut(no_hi nh) const
{
    if (empty() || get() == "HI") {
        if (nh == no_hi::no)
            return "hi";
        else
            return "";
    }
    else if (get() == "HINT")
        return "hint";
    else
        return "neut";
}

// ----------------------------------------------------------------------

std::string ae::chart::v3::Assay::HI_or_Neut(no_hi nh) const
{
    if (empty() || get() == "HI") {
        if (nh == no_hi::no)
            return "HI";
        else
            return "";
    }
    else if (get() == "HINT")
        return "HINT";
    else
        return "Neut";
}

// ----------------------------------------------------------------------

std::string ae::chart::v3::Assay::short_name() const
{
    if (get() == "FOCUS REDUCTION")
        return "FRA";
    else
        return get();
}

// ----------------------------------------------------------------------
