import sys, pprint
import inspect
from typing import Callable
import ae_backend
import ae.utils.org

# ======================================================================

def style(chart: ae_backend.chart_v3.Chart, style_name: str, antigen_selector: Callable = None, serum_selector: Callable = None, style_priority=0, modifier: dict = {"size": 30, "raise": True, "fill": "orange", "outline": "black", "label": {"size": 10}}):
    style = chart.styles()[style_name]
    style.priority = style_priority
    selected : list[dict[str, object]] = []
    if antigen_selector is not None:
        for no, ag in chart.select_antigens(antigen_selector):
            mod = {**modifier}
            _add_label(chart=chart, point=ag, mod=mod)
            style.add_modifier(selector={"!i": no}, only="antigens", **mod)
            selected.append({"ag": "AG", "no": no, "designation": ag.designation()})
    if serum_selector is not None:
        for no, sr in chart.select_sera(serum_selector):
            mod = {**modifier}
            _add_label(chart=chart, point=ag, mod=mod)
            style.add_modifier(selector={"!i": no}, only="sera", **mod)
            selected.append({"sr": "SR", "no": no, "designation": sr.designation()})
    print(f">>>> {inspect.getsource(antigen_selector).strip()}", file=sys.stderr)
    print(ae.utils.org.dict_to_org_table(selected, field_order=["ag", "no", "designation"], add_org_mode_wrapper=False), file=sys.stderr)

# ----------------------------------------------------------------------

def _add_label(chart: ae_backend.chart_v3.Chart, point: ae_backend.chart_v3.Antigen|ae_backend.chart_v3.Serum, mod: dict):
    if (label := mod.get("label")) and "text" not in label:
        label["text"] = point.designation()

# ======================================================================
