import sys, pprint, json
from typing import Optional
import ae_backend
from ..utils.num_digits import num_digits
from .. import virus
from . import name_passage
from .name_generator import NameGenerator

# ======================================================================

sPassages = ["cell", "egg", "reassortant"]

class Vaccine:

    def __init__(self, name: str, year: str, surrogate: bool):
        self.name = name
        self.year = year
        self.surrogate = surrogate
        for passage in sPassages:
            setattr(self, passage, [])

    def __bool__(self):
        return any(bool(getattr(self, passage, None)) for passage in sPassages)

    class Entry:

        def __init__(self, chart: ae_backend.chart_v3.Chart, no: int, antigen: ae_backend.chart_v3.Antigen):
            self.no = no
            self.antigen = antigen
            self.layers = chart.titers().layers_with_antigen(no)

        def __str__(self):
            return f"{self.no:4d} {self.antigen.designation()} (layers: {len(self.layers)})"

        def to_dict(self):
            return {"no": self.no, "designation": self.antigen.designation(), "layers": self.layers}

    def semantic_vaccine(self, entry: Entry, current_vaccine_years: list[str] = []):
        if entry.antigen.reassortant():
            passage = "reassortant"
        elif entry.antigen.passage().is_egg():
            passage = "egg"
        elif entry.antigen.passage().is_cell():
            passage = "cell"
        else:
            passage = " "       # space!
        val = f"{self.year or ''}{passage[0]}{self.surrogate or ''}"
        if self.year in current_vaccine_years:
            val += "c"
        entry.antigen.semantic.vaccine(val)

    def __repr__(self):
        return (json.dumps({"name": self.name, "year": self.year, "surrogate": self.surrogate, "cell": [en.to_dict() for en in self.cell], "egg": [en.to_dict() for en in self.egg], "reassortant": [en.to_dict() for en in self.reassortant]}))

    def report(self):
        print(f"{self.name} [{self.year}]{' <surrogate>' if self.surrogate else ''}", file=sys.stderr)
        for passage_type in sPassages:
            for no, en in enumerate(getattr(self, passage_type, [])):
                if no == 0:
                    prefix = f"{passage_type[:4]:<4s}"
                else:
                    prefix = " " * 4
                print(f"  {prefix}  {en}", file=sys.stderr)
        print(file=sys.stderr)

    @classmethod
    def make(cls, chart: ae_backend.chart_v3.Chart, name: str, year: str = None, passage: str = None, surrogate: bool = False, **ignored):
        chart_has_layers = chart.titers().number_of_layers() > 0
        layout = chart.projection().layout() if chart.number_of_projections() else None # avoid disconnected if projection present
        vac_name = virus.add_subtype_prefix(chart, name)
        vaccine = Vaccine(name=name, year=year, surrogate=surrogate)
        for psg in [passage] if passage else sPassages:
            antigens = chart.select_antigens(lambda ag: ag.name == vac_name and not ag.distinct() and ag.passage_is(psg) and layout.connected(ag.point_no))
            if len(antigens):
                if chart_has_layers and len(antigens) > 1:
                    antigens.sort_by_number_of_layers_descending()
                setattr(vaccine, psg, [cls.Entry(chart, *antigen) for antigen in antigens])
        return vaccine

# ----------------------------------------------------------------------

def find(chart: ae_backend.chart_v3.Chart, semantic_attribute_data: list, report: bool = True) -> list[Vaccine]:
    """Return list of vaccine entries (one entry per name).
    semantic_attribute_data is loaded from e.g. acmacs-data/semantic-vaccines.py
    order of returned elements is the same as entries
    """
    data = [en for en in (Vaccine.make(chart, **source) for source in semantic_attribute_data) if en]
    if report:
        for en in data:
            en.report()
    return data

# ======================================================================

def collect_data_for_styles(chart: ae_backend.chart_v3.Chart):
    """Look for "V" semantic attribute to collect data for styles"""
    name_generator = NameGenerator()
    vaccine_data = [{
        "no": no,
        "designation": antigen.designation(),
        "lox": 0.0, "loy": 1.0,     # label offset
        "size": 70.0,
        "outline_width": 4.0,
        "label_size": 36.0,
        "label": name_generator.location_year2_passaga_type(antigen),
        "semantic": antigen.semantic.get("V")
    } for no, antigen in chart.select_antigens(lambda ag: bool(ag.antigen.semantic.get("V")))]
    vaccine_data.sort(key=lambda en: en["semantic"] + en["designation"])
    return vaccine_data

# ----------------------------------------------------------------------

def default_field_order():
    return ["no", "designation", "size", "lox", "loy", "label", "label_size", "semantic", "outline_width"]

# ----------------------------------------------------------------------

def update(collected: list[dict[str, object]], user: list[dict[str, object]], match_by: str = "designation") -> list[dict[str, object]]:
    """match_by is a unique key! user data order is preferred, extra collected data is at the beginning of the result"""
    collected_ref = {en[match_by]: en for en in collected}
    user_ref = {en[match_by]: en for en in user}

    def upd(src: dict[str, object], upd: dict[str, object]) -> dict[str, object]:
        res = {**src}
        for key, val in upd.items():
            if key == "no":
                if str(val) != str(res[key]):
                    print(f">> vaccine.update: no mismatch (\"{val}\" vs. \"{res[key]}\") for matching key: {src} -- {upd}", file=sys.stderr)
            elif key != match_by and val is not None and val != "":
                res[key] = val
        return res
    user_result = [upd(collected_ref.get(usr[match_by], {match_by: usr[match_by]}), usr) for usr in user]
    collected_not_in_user = [coll for coll in collected if coll[match_by] not in user_ref]
    return collected_not_in_user + user_result

# ======================================================================

sModifier = {"outline": "black", "fill": ":bright", "raise": True, "size": 70, "only": "antigens"}
sLabelModifier = {"offset": [0, 1], "slant": "normal", "weight": "normal", "size": 36.0, "color": "black"}

def style(chart: ae_backend.chart_v3.Chart, style_name: str, data: list[dict[str, object]], common_modifier: dict, label_modifier: dict, priority: int = 1000) -> set[str]:
    """Add "-vaccines" and "-vaccines-blue" plot style
    data is the output of find() and/or update()
    common_modifier: {"outline": "black", "fill": ":bright", "size": 70}
    label_modifier: {"slant": "normal", "weight": "normal", "color": "black"} - other fields are taken from data
    """

    style = chart.styles()[style_name]
    style.priority = 1000
    style.add_modifier(selector={"V": True}, only="antigens", raise_=True, **common_modifier)

    for en in data:
        style.add_modifier(selector={"!i": en["no"]}, only="antigens", **_extract_point_modifier_data(source=en, label_modifier=label_modifier))
    return {style_name}

# ----------------------------------------------------------------------

def _extract_point_modifier_data(source: dict[str, object], label_modifier: dict) -> dict[str, object]:
    def get_float(key: str) -> float:
        if (val := source.get(key)) is not None:
            return float(val)
        else:
            return None

    def get_bool(key: str) -> bool:
        if (val := source.get(key)) is not None:
            return val.lower() in ["t", "true", "y", "yes", "1"]
        else:
            return None

    return _remove_none({
        "fill": source.get("fill"),
        "outline": source.get("outline"),
        "outline_width": get_float("outline_width"),
        "size": get_float("size"),
        "rotation": get_float("rotation"),
        "aspect": get_float("aspect"),
        "shape": source.get("shape"),
        "hide": get_bool("hide"),
        "label": {
            **label_modifier,
            **_remove_none({
                "offset": [get_float("lox") or 0.0, get_float("loy") or 0.0],
                "text": source.get("label"),
                "size": get_float("label_size"),
                "color": source.get("label_color"),
                "rotation": get_float("label_rotation"),
                "shown": source.get("label_shown"),
                "slant": source.get("label_slant"),
                "weight": source.get("label_weight"),
            })
        }
    })

# ----------------------------------------------------------------------

def _remove_none(source: dict):
    return {key: val for key, val in source.items() if val is not None}

    # def make_style(name: str, point_modifier: dict):
    #     style = chart.styles()[name]
    #     style.priority = 1000
    #     style.add_modifier(selector={"V": True}, **point_modifier)
    #     return style

    # name_generator = NameGenerator()
    # styles = [make_style(name=style_name, point_modifier=sModifier), make_style(name=style_name + "-blue", point_modifier={**sModifier, "fill": "blue"})]
    # # individual vaccine modifiers with label data
    # for no, antigen in chart.select_antigens(lambda ag: bool(ag.antigen.semantic.get("V"))):
    #     for style in styles:
    #         style.add_modifier(selector={"!i": no}, only="antigens", outline_width=4.0, label={**sLabelModifier, "text": name_generator.location_year2_passaga_type(antigen)})

    # def is_current_vaccine(ag):
    #     if vaccine_attribute := ag.antigen.semantic.get("V"):
    #         return vaccine_attribute[-1] == "c"
    #     else:
    #         return False

    # for no, antigen in chart.select_antigens(lambda ag: is_current_vaccine(ag)):
    #     fill = "green" if antigen.reassortant() else "red"
    #     styles[1].add_modifier(selector={"!i": no}, only="antigens", fill=fill)

    # return set([style_name, style_name + "-blue"])

# ======================================================================
# ======================================================================
# ======================================================================

# class Result (name_passage.Result):

#     def _format_header(self, name: str, en: dict, **args):
#         surrogate = " surrogate" if en.get("surrogate") else ""
#         return f"Vaccine {name}{surrogate} [{en['year']}]"

#     def _sorting_key(self, name: str):
#         return self.data[name].get("year") or ""

# # ----------------------------------------------------------------------

# def attributes(chart: ae_backend.chart_v3.Chart, semantic_attribute_data: list, current_vaccine_years: list[str] = [], report: bool = False):
#     """semantic_attribute_data are returned by acmacs-data/semantic-vaccines semantic_data_for_subtype(subtype).
#     [{"name": "MALAYSIA/2506/2004", "passage": "egg|cell|reassortant", "surrogate": False, "year": "2006", **ignored}]
#     """
#     result = Result(chart)
#     for en in semantic_attribute_data:
#         _semantic_entry(chart=chart, result=result, current_vaccine_years=current_vaccine_years, **en)
#     if report:
#         print(result.report(), file=sys.stderr)
#     return result

# # ----------------------------------------------------------------------

# def _semantic_entry(chart: ae_backend.chart_v3.Chart, name: str, year: str = None, passage: str = None, surrogate: bool = False, result: Result = None, current_vaccine_years: list[str] = None, **ignored) -> Result:
#     layout = chart.projection().layout() if chart.number_of_projections() else None # avoid disconnected if projection present
#     vac_name = virus.add_subtype_prefix(chart, name)
#     if result is None:
#         result = Result(chart)
#     subreport = {}
#     for psg in _passages(passage):
#         antigens = chart.select_antigens(lambda ag: ag.name == vac_name and not ag.distinct() and ag.passage_is(psg) and layout.connected(ag.point_no))
#         if len(antigens):
#             if result._has_layers and len(antigens) > 1:
#                 antigens.sort_by_number_of_layers_descending()
#             antigens[0][1].semantic.vaccine(_make_attribute_value(year=year, current_vaccine_years=current_vaccine_years, passage=passage, surrogate=surrogate, antigen=antigens[0][1]))
#             subreport[psg] = antigens
#     if subreport:
#         result.data[name] = {"year": year, "surrogate": surrogate, **subreport}
#     return result

# # ----------------------------------------------------------------------

# def _make_attribute_value(year: Optional[str], current_vaccine_years: list[str], passage: str, surrogate: bool, antigen: ae_backend.chart_v3.Antigen):
#     val = ""
#     if not passage:
#         if antigen.reassortant():
#             passage = "reassortant"
#         elif antigen.passage().is_egg():
#             passage = "egg"
#         elif antigen.passage().is_cell():
#             passage = "cell"
#     if passage:
#         val += passage[0]
#     if surrogate:
#         val += "s"
#     val += year or ""
#     if year in current_vaccine_years:
#         val += "c"
#     return val

# # ----------------------------------------------------------------------

# def _passages(passage_type: Optional[str]):
#     if passage_type:
#         return [passage_type]
#     else:
#         return sPassages

# # ======================================================================

# sModifier = {"outline": "black", "fill": ":bright", "raise": True, "size": 70, "only": "antigens"}
# sLabelModifier = {"offset": [0, 1], "slant": "normal", "weight": "normal", "size": 36.0, "color": "black"}

# def style(chart: ae_backend.chart_v3.Chart, style_name: str = "-vaccines") -> set[str]:
#     """Add "-vaccines" and "-vaccines-blue" plot style"""

#     def make_style(name: str, point_modifier: dict):
#         style = chart.styles()[name]
#         style.priority = 1000
#         style.add_modifier(selector={"V": True}, **point_modifier)
#         return style

#     name_generator = NameGenerator()
#     styles = [make_style(name=style_name, point_modifier=sModifier), make_style(name=style_name + "-blue", point_modifier={**sModifier, "fill": "blue"})]
#     # individual vaccine modifiers with label data
#     for no, antigen in chart.select_antigens(lambda ag: bool(ag.antigen.semantic.get("V"))):
#         for style in styles:
#             style.add_modifier(selector={"!i": no}, only="antigens", outline_width=4.0, label={**sLabelModifier, "text": name_generator.location_year2_passaga_type(antigen)})

#     def is_current_vaccine(ag):
#         if vaccine_attribute := ag.antigen.semantic.get("V"):
#             return vaccine_attribute[-1] == "c"
#         else:
#             return False

#     for no, antigen in chart.select_antigens(lambda ag: is_current_vaccine(ag)):
#         fill = "green" if antigen.reassortant() else "red"
#         styles[1].add_modifier(selector={"!i": no}, only="antigens", fill=fill)

#     return set([style_name, style_name + "-blue"])

# ======================================================================
