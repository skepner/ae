import sys
import ae_backend

# ======================================================================

def attributes(chart: ae_backend.chart_v3.Chart):
    """Set serum circle semantic attribute for all sera"""
    for fold in [2.0, 4.0, 8.0]:
        for circle_data in chart.projection().serum_circles(fold=fold):
            attr = {"cb": circle_data.column_basis}
            if empirical := circle_data.empirical():
                attr["e"] = empirical
            if theoretical := circle_data.theoretical():
                attr["t"] = theoretical
            chart.serum(circle_data.serum_no).semantic.set(f"CI{int(fold)}", attr)
            # print(f">>>> SR {circle_data.serum_no:3d} {chart.serum(circle_data.serum_no).designation():40s} {chart.serum(circle_data.serum_no).semantic}", file=sys.stderr)

# ======================================================================