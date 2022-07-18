import ae_backend

# ======================================================================

def style(chart: ae_backend.chart_v3.Chart, style_name: str = "-pale", priority: int = 1000) -> set[str]:
    """Add "-pale" plot style"""
    style = chart.styles()[style_name]
    style.priority = priority
    style.add_modifier(outline=":pale", fill=":pale")
    return set([style_name])

# ======================================================================
