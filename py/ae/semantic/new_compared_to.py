import sys
import ae_backend

# ======================================================================

def attributes(chart: ae_backend.chart_v3.Chart, previous_chart: ae_backend.chart_v3.Chart, new_attribute_value: int):
    """Set "new" attribute to new_attribute_value for antigens in chart that are not in previous_chart."""
    new_antigens = chart.select_new_antigens(previous_chart)
    # print(f">>>> new_antigens {new_antigens}", file=sys.stderr)
    for no, ag in new_antigens:
        ag.semantic.set("new", new_attribute_value)

# ======================================================================
