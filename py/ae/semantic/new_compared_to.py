import sys
import ae_backend
from .style import style_with_one_modifier

# ======================================================================

def attributes(chart: ae_backend.chart_v3.Chart, previous_chart: ae_backend.chart_v3.Chart, new_attribute_value: int):
    """Set "new" attribute to new_attribute_value for antigens in chart that are not in previous_chart."""
    new_antigens = chart.select_new_antigens(previous_chart)
    # print(f">>>> new_antigens {new_antigens}", file=sys.stderr)
    for no, ag in new_antigens:
        ag.semantic.set("new", new_attribute_value)

# ======================================================================

def style_new(chart: ae_backend.chart_v3.Chart, number_of_previous_charts: int, first_priority: int = 4500):
    """add "-new-1", "-new-2", "-new-1-big" styles marking antigens with the "new" semantic attribute"""
    if number_of_previous_charts > 0:
        priority = first_priority
        outline_widths = [1, 6, 3]
        for prev_no in range(number_of_previous_charts, 0, -1):
            style_with_one_modifier(chart=chart, style_name=f"-new-{prev_no}", selector={"new": prev_no}, modifier={"outline": "black", "outline_width": outline_widths[prev_no], "only": "antigens", "raise": True}, priority=priority)
            priority += 1

# ======================================================================
