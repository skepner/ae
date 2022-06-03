import sys
import ae_backend

# ======================================================================

def style(chart: ae_backend.chart_v3.Chart, style_name: str, serum_no: int, priority: int = 100, fold: float = 2.0, theoretical: bool = False, within: dict = {"outline": "pink", "fill": ":bright", "outline_width": 3}, outside: dict = {"outline": "black", "fill": ":bright", "outline_width": 3}) -> set[str]:
    """
    empirical: True - show empirical, False - show theoretical.
    within: {"outline": "pink", "fill": ":bright", "outline_width": 3}
    outside: {"outline": "black", "fill": ":bright", "outline_width": 3}
    """
    style = chart.styles()[style_name]
    style.priority = priority
    style.add_modifier(selector={"!i": serum_no}, only="sera", serum_coverage={"fold": fold, "theoretical": theoretical, "within": within, "outside": outside})
    return set([style_name])

# ======================================================================
