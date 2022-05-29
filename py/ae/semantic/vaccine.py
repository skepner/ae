import sys, pprint
from typing import Optional
import ae_backend
from ..utils.num_digits import num_digits
from .. import virus
from . import name_passage
from .name_generator import NameGenerator

# ======================================================================

sPassages = ["cell", "egg", "reassortant"]

class Result (name_passage.Result):

    def _format_header(self, name: str, en: dict, **args):
        surrogate = " surrogate" if en.get("surrogate") else ""
        return f"Vaccine {name}{surrogate} [{en['year']}]"

    def _sorting_key(self, name: str):
        return self.data[name]["year"]

# ----------------------------------------------------------------------

def attributes(chart: ae_backend.chart_v3.Chart, entries: list, current_vaccine_years: list[str] = []):
    """entries are returned by acmacs-data/semantic-vaccines semantic_data_for_subtype(subtype).
    [{"name": "MALAYSIA/2506/2004", "passage": "egg|cell|reassortant", "surrogate": False, "year": "2006", **ignored}]
    """
    result = Result(chart)
    for en in entries:
        _semantic_entry(chart=chart, result=result, current_vaccine_years=current_vaccine_years, **en)
    return result

# ----------------------------------------------------------------------

def _semantic_entry(chart: ae_backend.chart_v3.Chart, name: str, year: str = None, passage: str = None, surrogate: bool = False, result: Result = None, current_vaccine_years: list[str] = None, **ignored) -> Result:
    layout = chart.projection().layout() if chart.number_of_projections() else None # avoid disconnected if projection present
    vac_name = virus.add_subtype_prefix(chart, name)
    if result is None:
        result = Result(chart)
    subreport = {}
    for psg in _passages(passage):
        antigens = chart.select_antigens(lambda ag: ag.name == vac_name and ag.passage_is(psg) and layout.connected(ag.point_no))
        if len(antigens):
            if result._has_layers and len(antigens) > 1:
                antigens.sort_by_number_of_layers_descending()
            antigens[0][1].semantic.vaccine(_make_attribute_value(year=year, current_vaccine_years=current_vaccine_years, passage=passage, surrogate=surrogate, antigen=antigens[0][1]))
            subreport[psg] = antigens
    if subreport:
        result.data[name] = {"year": year, "surrogate": surrogate, **subreport}
    return result

# ----------------------------------------------------------------------

def _make_attribute_value(year: Optional[str], current_vaccine_years: list[str], passage: str, surrogate: bool, antigen: ae_backend.chart_v3.Antigen):
    val = ""
    if not passage:
        if antigen.reassortant():
            passage = "reassortant"
        elif antigen.passage().is_egg():
            passage = "egg"
        elif antigen.passage().is_cell():
            passage = "cell"
    if passage:
        val += passage[0]
    if surrogate:
        val += "s"
    val += year or ""
    if year in current_vaccine_years:
        val += "c"
    return val

# ----------------------------------------------------------------------

def _passages(passage_type: Optional[str]):
    if passage_type:
        return [passage_type]
    else:
        return sPassages

# ======================================================================

sModifier = {"outline": "black", "fill": ":bright", "raise": True, "size": 70, "only": "antigens"}
sLabelModifier = {"offset": [0, 1], "slant": "normal", "weight": "normal", "size": 36.0, "color": "black"}

def style(chart: ae_backend.chart_v3.Chart, style_name: str = "-vaccines") -> set[str]:
    """Add "-vaccines" and "-vaccines-blue" plot style"""

    def make_style(name: str, point_modifier: dict):
        style = chart.styles()[name]
        style.priority = 1000
        style.add_modifier(selector={"V": True}, **point_modifier)
        return style

    name_generator = NameGenerator()
    styles = [make_style(name=style_name, point_modifier=sModifier), make_style(name=style_name + "-blue", point_modifier={**sModifier, "fill": "blue"})]
    # individual vaccine modifiers with label data
    for no, antigen in chart.select_antigens(lambda ag: bool(ag.antigen.semantic.get("V"))):
        for style in styles:
            style.add_modifier(selector={"!i": no}, outline_width=4.0, label={**sLabelModifier, "text": name_generator.location_year2_passaga_type(antigen)})

    def is_current_vaccine(ag):
        if vaccine_attribute := ag.antigen.semantic.get("V"):
            return vaccine_attribute[-1] == "c"
        else:
            return False

    for no, antigen in chart.select_antigens(lambda ag: is_current_vaccine(ag)):
        fill = "green" if antigen.reassortant() else "red"
        styles[1].add_modifier(selector={"!i": no}, fill=fill)

    return set([style_name, style_name + "-blue"])

# ======================================================================
