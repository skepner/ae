import sys
import ae_backend

# ======================================================================

def semantic(chart: ae_backend.chart_v3.Chart):
    """Set continent ("C9") and country ("c9") semantic attributes for all antigens and sera"""
    for selector in [chart.select_all_antigens, chart.select_all_sera]:
        for ag_no, antigen in selector():
            if parsed := ae_backend.virus.name_parse(antigen.name()):
                if parsed.parts.continent:
                    antigen.semantic.set("C9", parsed.parts.continent)
                if parsed.parts.country:
                    antigen.semantic.set("c9", parsed.parts.country)

# ======================================================================
