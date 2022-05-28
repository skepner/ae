import sys
import ae_backend

# ======================================================================

def attributes(chart: ae_backend.chart_v3.Chart):
    """Set passage type ("p") semantic attributes for all antigens and sera"""
    for selector in [chart.select_all_antigens, chart.select_all_sera]:
        for ag_no, antigen in selector():
            pt = None
            if antigen.reassortant():
                pt = "r"
            elif (passage := antigen.passage()).is_egg():
                pt = "e"
            elif passage.is_cell():
                pt = "c"
            if pt:
                antigen.semantic.set("p", pt)

# ======================================================================
