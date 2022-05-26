import sys, pprint
import ae_backend
from ..utils.num_digits import num_digits
from .. import virus

# ======================================================================

sPassages = ["cell", "egg", "reassortant"]

class Result:

    def __init__(self, chart: ae_backend.chart_v3.Chart):
        self.data = {}        # {name: {"year": year, "surrogate": False, passage: [antigens]}}
        self.chart = chart
        self._ag_no_num_digits = num_digits(self.chart.number_of_antigens())
        self._has_layers = self.chart.titers().number_of_layers() > 0

    def __bool__(self):
        return bool(self.data)

    def report(self) -> str:
        if not self.data:
            return ""
        return "\n".join(self._format_entry(name) for name in sorted(self.data, key=self._sorting_key))

    def _sorting_key(self, name: str):
        return self.data[name]["year"]

    def _format_entry(self, name: str):
        en = self.data[name]
        surrogate = " surrogate" if en["surrogate"] else ""
        subentries = "\n  ".join(f"{passage[:3]} " + "\n      ".join(self._format_subentry(passage=passage, antigens=en[passage])) for passage in sPassages if passage in en)
        return f"Vaccine {name}{surrogate} [{en['year']}]\n  {subentries}"

    def _format_subentry(self, passage: str, antigens: list):
        for no, ag in antigens:
            if self._has_layers:
                layers = self.chart.titers().layers_with_antigen(no)
                layers_s = f" layers: ({len(layers)}){layers}"
            else:
                layers = []
                layers_s = ""
            yield f"AG {no:{self._ag_no_num_digits}d} {ag.designation()}{layers_s}"

# ----------------------------------------------------------------------

def semantic(chart: ae_backend.chart_v3.Chart, entries: list):
    """entries are returned by acmacs-data/semantic-vaccines semantic_data_for_subtype(subtype).
    [{"name": "MALAYSIA/2506/2004", "passage": "egg|cell|reassortant", "surrogate": False, "year": "2006", **ignored}]
    """
    result = Result(chart)
    for en in entries:
        _semantic_entry(chart=chart, result=result, **en)
    return result

# ----------------------------------------------------------------------

def _semantic_entry(chart: ae_backend.chart_v3.Chart, name: str, year: str = None, passage: str = None, surrogate: bool = False, result: Result = None, **ignored) -> Result:
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
            antigens[0][1].semantic.vaccine(_make_attribute_value(year=year, passage=passage, surrogate=surrogate, antigen=antigens[0][1]))
            subreport[psg] = antigens
    if subreport:
        result.data[name] = {"year": year, "surrogate": surrogate, **subreport}
    return result

# ----------------------------------------------------------------------

def _make_attribute_value(year: str, passage: str, surrogate: bool, antigen: ae_backend.chart_v3.Antigen):
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
    return val

# ----------------------------------------------------------------------

def _passages(passage_type: str):
    if passage_type:
        return [passage_type]
    else:
        return sPassages

# ======================================================================

def plot_style(chart: ae_backend.chart_v3.Chart, name: str = "-vaccines") -> set[str]:
    """Add "-vaccines" plot style"""
    modifier = {"outline": "black", "raise": True, "size": 70, "only": "antigens"}
    style = chart.styles()[name]
    style.priority = 1000
    style.add_modifier(selector={"V": True}, **modifier)
    # individual vaccine modifiers with label data
    locdb = ae_backend.locdb_v3.locdb()
    for no, antigen in chart.select_antigens(lambda ag: bool(ag.antigen.semantic.get("V"))):
        if reassortant := antigen.reassortant():
            passage_type = f"{reassortant}-egg"
        else:
            passage_type = antigen.passage().passage_type()
        if name_parsing_result := ae_backend.virus.name_parse(antigen.name()):
            name = f"{locdb.abbreviation(name_parsing_result.parts.location)}/{name_parsing_result.parts.year[2:]}"
        else:
            name = antigen.name()
        style.add_modifier(selector={"!i": no}, outline_width=4.0, label={"offset": [0, -1], "text": f"{name}-{passage_type}", "slant": "italic", "weight": "bold", "size": 19.0, "color": "red"})
        # print(f">>>> {no} {antigen.designation()}", file=sys.stderr)
    return set([name])

# ----------------------------------------------------------------------

# def vaccine(chart: ae_backend.chart_v3.Chart, name: str, year: str = None, passage: str = None, surrogate: bool = False, result: Result = None, **ignored) -> Result:
#     """Add "V" semantic attribute to the vaccine antigens.
#     passage: "egg", "cell", "reassortant"
#     """
#     # print(f"> {name} {passage} [{year}]")
#     layout = chart.projection().layout() if chart.number_of_projections() else None
#     vac_name = virus.add_subtype_prefix(chart, name)
#     if result is None:
#         result = Result(chart)
#     subreport = {}
#     for psg in (sPassages if passage is None else [passage]):
#         antigens = chart.select_antigens(lambda ag: ag.name == vac_name and ag.passage_is(psg) and layout.connected(ag.point_no))
#         if len(antigens):
#             if result._has_layers and len(antigens) > 1:
#                 antigens.sort_by_number_of_layers_descending()
#             antigens[0][1].semantic.vaccine((year or "") + ("s" if surrogate else ""))
#             subreport[psg] = antigens
#     if subreport:
#         result.data[name] = {"year": year, "surrogate": surrogate, **subreport}
#     return result

# # ----------------------------------------------------------------------

# def vaccines(chart: ae_backend.chart_v3.Chart, entries: list) -> Result:
#     result = Result(chart)
#     for en in entries:
#         vaccine(chart=chart, result=result, **en)
#     return result

# ----------------------------------------------------------------------

# def vaccines_and_plot_style(chart: ae_backend.chart_v3.Chart, entries: list, modifier: dict = {"outline": "black", "rais": True, "size": 70, "only": "antigens"}, report: bool = True) -> str:
#     if result := vaccines(chart=chart, entries=entries):
#         style_name = "-vaccines"
#         style = chart.styles()[style_name]
#         style.priority = 1000
#         style.add_modifier(selector=["V", ""], **modifier)
#         if report:
#             print(result.report(), file=sys.stderr)
#         return style_name
#     else:
#         return None

# ======================================================================
