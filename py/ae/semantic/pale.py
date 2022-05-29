import ae_backend

# ======================================================================

def style(chart: ae_backend.chart_v3.Chart, style_name: str = "-pale") -> set[str]:
    """Add "-pale" plot style"""
    style = chart.styles()[style_name]
    style.priority = 1000
    style.add_modifier(outline=":pale", fill=":pale")
    return set([style_name])

# ======================================================================
