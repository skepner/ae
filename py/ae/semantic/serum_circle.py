import sys
import ae_backend

# ======================================================================

def attributes(chart: ae_backend.chart_v3.Chart):
    """Set serum circle semantic attribute for all sera"""
    for fold in [2.0, 3.0]:
        for circle_data in chart.projection().serum_circles(fold=fold):
            attr = {"cb": circle_data.column_basis}
            if empirical := circle_data.empirical():
                attr["e"] = empirical
            if theoretical := circle_data.theoretical():
                attr["t"] = theoretical
            chart.serum(circle_data.serum_no).semantic.set(f"CI{int(fold)}", attr)
            # print(f">>>> SR {circle_data.serum_no:3d} {chart.serum(circle_data.serum_no).designation():40s} {chart.serum(circle_data.serum_no).semantic}", file=sys.stderr)

# ======================================================================

def style(chart: ae_backend.chart_v3.Chart, style_name: str, priority: int = 100, sera: list[int] | None = None, fold: float = 2.0, theoretical: bool = False, fallback: bool = True, circle_style: dict = {"outline": {"egg": "red", "cell": "blue", "reassortant": "orange"}, "fill": {"egg": "transparent", "cell": "transparent", "reassortant": "transparent"}, "outline_width": 1.0, "dash": 0}) -> set[str]:
    """If sera is None show circles for all sera (if semantic attribute data is available), otherwise it's a list of serum indexes.
    empirical: True - show empirical, False - show theoretical.
    fallback: True - show fallback circle if empirical/theoretical is not available.
    circle_style: {
      "outline": {"egg": "red", "cell": "blue", "reassortant": "orange"},
      "fill": {"egg": "transparent", "cell": "transparent", "reassortant": "transparent"},
      "outline_width": 1.0,
      "dash": 0,
      "angles": None, # two angles to show radius lines and fill between lines only
      "radius_lines": {"outline": {}, "outline_width": 1.0, "dash": 0}
    }
    """
    style = chart.styles()[style_name]
    style.priority = priority
    num_sera = chart.number_of_sera()
    sera = sera if sera is not None else list(range(num_sera))
    for serum_no in sera:
        if serum_no >= 0 and serum_no < num_sera:
            serum_passage_type = chart.serum(serum_no).passage().passage_type()
            this_circle_style = {**circle_style}
            if isinstance(this_circle_style.get("outline"), dict):
                this_circle_style["outline"] = this_circle_style["outline"][serum_passage_type]
            if isinstance(this_circle_style.get("fill"), dict):
                this_circle_style["fill"] = this_circle_style["fill"][serum_passage_type]
            if isinstance(this_circle_style.get("radius_lines", {}).get("outline"), dict):
                this_circle_style["radius_lines"]["outline"] = this_circle_style["radius_lines"]["outline"][serum_passage_type]
            style.add_modifier(selector={"!i": serum_no}, only="sera", serum_circle={"fold": fold, "theoretical": theoretical, "fallback": fallback, "style": this_circle_style})
        else:
            print(f">> serum_circle.style: invalid serum no {serum_no}, number of sera in the chart: {num_sera}", file=sys.stderr)
    return set([style_name])

# ======================================================================
