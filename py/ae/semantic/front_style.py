import ae_backend

# ======================================================================

def add(chart: ae_backend.chart_v3.Chart, style_name: str, references: list[str], title: str, title_style: dict[str, object] = {}, show_legend: bool = True, legend_counter: bool = True, style_priority: int = 1):
    style = chart.styles()[style_name]
    style.priority = style_priority
    for ref in references:
        style.add_modifier(parent=ref)
        style.plot_title.text.text = title
        _title_style(plot_title=style.plot_title, title_style=title_style)
        style.legend.shown = show_legend
        style.legend.add_counter = legend_counter

# ----------------------------------------------------------------------

def _title_style(plot_title, title_style: dict[str, object]):
    for tkey, skey in [["font_size", "size"], ["font_weight", "weight"], ["font_slant", "slant"], ["font_face", "face"], ["color", "color"], ["interline", "interline"]]:
        if (value := title_style.get(skey)) is not None:
            setattr(plot_title.text, tkey, value)
    for tkey, skey in [["origin", "origin"], ["padding", "padding"], ["border_color", "border_color"], ["border_width", "border_width"], ["background_color", "background"]]:
        if (value := title_style.get(skey)) is not None:
            setattr(plot_title.box, tkey, value)
    if offset := title_style.get("offset"):
        plot_title.box.offset(*offset)

# ======================================================================
