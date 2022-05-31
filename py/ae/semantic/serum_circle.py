import sys
import ae_backend

# ======================================================================

def attributes(chart: ae_backend.chart_v3.Chart, fold: float = 2.0):
    """Set serum circle semantic attribute for all sera"""
    for circle_data in chart.projection().serum_circles(fold=fold):
        attr = {"cb": circle_data.column_basis}
        if empirical := circle_data.empirical():
            attr["e"] = empirical
        if theoretical := circle_data.theoretical():
            attr["t"] = theoretical
        chart.serum(circle_data.serum_no).semantic.set(f"CI{int(fold)}", attr)
        print(f">>>> SR {circle_data.serum_no:3d} {chart.serum(circle_data.serum_no).designation():40s} {attr}", file=sys.stderr)

# ======================================================================
