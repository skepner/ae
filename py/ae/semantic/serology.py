import sys, pprint
from typing import Optional
import ae_backend
from .find import AntigenFinder, PASSAGES
from .name_generator import NameGenerator
from .vaccine import update, extract_point_modifier_data # update called by 0do via this module

# ======================================================================

def find(chart: ae_backend.chart_v3.Chart, semantic_attribute_data: list[dict[str, str]], report: bool = True) -> list[dict[str, object|list[dict[str, object]]]]:
    """Return list of serology antigen entries (one entry per name).
    semantic_attribute_data [{"name": "Wisconsin/588/2019", "passage": "cell"}] is loaded from e.g. acmacs-data/semantic-serology.py
    order of returned elements is the same as entries
    """
    # pprint.pprint(semantic_attribute_data, width=200)
    finder = AntigenFinder(chart)
    data = [en for en in ({"name": source["name"], **finder.find(name=source["name"], passage=source.get("passage"))} for source in semantic_attribute_data) if any(bool(en.get(psg)) for psg in PASSAGES)]
    if report:
        _report(data)
    return data

# ----------------------------------------------------------------------

def _report(data: list[dict[str, object]]):
    print(">>> Serology", file=sys.stderr)
    for en in data:
        print(en["name"], file=sys.stderr)
        for passage_type in PASSAGES:
            for no, en in enumerate(en.get(passage_type, [])):
                if no == 0:
                    prefix = f"{passage_type[:4]:<4s}"
                else:
                    prefix = " " * 4
                print(f"  {prefix}  {en['no']:4d} {en['antigen'].designation()} (layers: {len(en['layers'])})", file=sys.stderr)
        print(file=sys.stderr)

# ======================================================================

def collect_data_for_styles(chart: ae_backend.chart_v3.Chart):
    """Look for "serology" semantic attribute to collect data for styles"""
    name_generator = NameGenerator()
    serology_data = [{
        "no": no,
        "designation": antigen.designation(),
        "lox": 0.0, "loy": 1.0,     # label offset
        # "size": 50.0,
        # "outline_width": 1.0,
        # "label_size": 28.0,
        "fill": "orange",
        "label": name_generator.location_isolation_year2_passaga_type(antigen),
    } for no, antigen in chart.select_antigens(lambda ag: bool(ag.antigen.semantic.get("serology")))]
    serology_data.sort(key=lambda en: en["designation"])
    # pprint.pprint(serology_data, width=200)
    return serology_data

# ----------------------------------------------------------------------

def default_field_order():
    return ["no", "designation", "lox", "loy", "fill", "label", "label_size", "size", "outline_width"]

# ======================================================================

def style(chart: ae_backend.chart_v3.Chart, style_name: str, data: list[dict[str, object]], common_modifier: dict, label_modifier: dict, priority: int = 1000) -> set[str]:
    """Add "-serology"
    data is the output of find() and/or update()
    common_modifier: {"outline": "black", "fill": ":bright", "size": 50, "outline_width": 1.0}
    label_modifier: {"size": 26.0, "slant": "normal", "weight": "normal", "color": "black"} - other fields are taken from data
    """

    style = chart.styles()[style_name]
    style.priority = 1000
    style.add_modifier(selector={"serology": True}, only="antigens", raise_=True, **common_modifier)

    for en in data:
        style.add_modifier(selector={"!i": en["no"]}, only="antigens", **extract_point_modifier_data(source=en, data_key_mapping={}, label_modifier=label_modifier))
    return {style_name}

# ======================================================================
