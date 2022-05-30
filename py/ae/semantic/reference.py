import sys
import ae_backend

# ======================================================================

def attributes(chart: ae_backend.chart_v3.Chart):
    """Set reference ("R") semantic attribute for reference antigens"""
    for ag_no, antigen in chart.select_reference_antigens():
        antigen.semantic.set("R", True)

# ======================================================================
