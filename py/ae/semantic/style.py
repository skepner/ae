import sys
import ae_backend

# ======================================================================

def style_with_one_modifier(chart: ae_backend.chart_v3.Chart, style_name: str, selector: dict[str, object], modifier: dict[str, object], priority: int) -> set[str]:
    style = chart.styles()[style_name]
    style.priority = priority
    style.add_modifier(selector=selector, **modifier)
    return set([style_name])

# ======================================================================
