import ae_backend

# ======================================================================

def add(chart: ae_backend.chart_v3.Chart, style_name: str, references: list[str], title: str, show_legend: bool, legend_counter: bool = True, style_priority: int = 1):
    style = chart.styles()[style_name]
    style.priority = style_priority
    for ref in references:
        style.add_modifier(parent=ref)
        style.plot_title.text.text = title
        style.legend.shown = show_legend
        style.legend.add_counter = legend_counter

# ======================================================================
