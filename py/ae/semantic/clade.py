import sys
from typing import Any

import ae_backend
from . import front_style

# ======================================================================

def attributes(chart: ae_backend.chart_v3.Chart, entries: list[dict[str, str]]):
    """expected entries: [{"name": "3C.2a1b.2a.2 156S", "clade": "3C.2a1b.2a.2", "aa": "156S", **ignored}]"""

    def set_by_clade_aa(name: str, clade: str, aa: str):
        for selector in [chart.select_antigens, chart.select_sera]:
            for no, ag_sr in selector(lambda en: en.has_clade(clade) and en.aa[aa]):
                ag_sr.semantic.add_clade(name)

    def set_by_aa(name: str, aa: str):
        for selector in [chart.select_antigens, chart.select_sera]:
            for no, ag_sr in selector(lambda en: en.aa[aa]):
                ag_sr.semantic.add_clade(name)

    for data in entries:
        if data.get("clade"):
            set_by_clade_aa(**data)
        else:
            set_by_aa(**data)

# ======================================================================

def style(chart: ae_backend.chart_v3.Chart, style_name: str, data: list[dict[str, str]], add_counter: bool = True, legend_style: dict[str, Any] = {}):
    """expected data: [{"name": "3C.2a1b.2a.2 156S", "legend": "2a1b.2a.2 156S", "color": "red", **ignored}]"""
    sname = f"-{style_name}-sera"
    style = chart.styles()[sname]
    style.priority = 1000
    for modifier in data:
        style.add_modifier(selector={"C": modifier["name"]}, outline=modifier["color"], outline_width=3.0, rais=True, only="sera")

    sname = f"-{style_name}"
    style = chart.styles()[sname]
    style.priority = 1000
    style.legend.add_counter = add_counter
    front_style.legend_style(style.legend, legend_style)
    legend_priority = 99
    for modifier in data:
        style.add_modifier(selector={"C": modifier["name"]}, fill=modifier["color"], outline="black", rais=True, only="antigens", legend=modifier["legend"], legend_priority=legend_priority)
        legend_priority -= 1

# ======================================================================
