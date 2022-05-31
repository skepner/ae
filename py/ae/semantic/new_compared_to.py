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

def style(chart: ae_backend.chart_v3.Chart, number_of_previous_charts: int, modifiers=list[dict], name_prefix: str = "-new", first_priority: int = 4500) -> set[str]:
    """add "-new-1" and "-new-2" style marking antigens with "new" semantic attribute"""
    snames = set()
    priority = first_priority
    for prev_no in range(number_of_previous_charts, 0, -1):
        sname = f"{name_prefix}-{prev_no}"
        style = chart.styles()[sname]
        style.priority = priority
        style.add_modifier(selector={"new": prev_no}, **modifiers[prev_no - 1], only="antigens")
        snames.add(sname)
        priority += 1
    return snames

# ======================================================================
