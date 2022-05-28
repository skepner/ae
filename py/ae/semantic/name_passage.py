import sys, pprint
import ae_backend
from ..utils.num_digits import num_digits
from .. import virus

# ======================================================================

sPassages = ["cell", "egg", "reassortant"]

class Result:

    def __init__(self, chart: ae_backend.chart_v3.Chart):
        self.data: dict[str, object] = {}        # {name: {"year": year, "surrogate": False, passage: [antigens]}}
        self.chart = chart
        self._ag_no_num_digits = num_digits(self.chart.number_of_antigens())
        self._has_layers = self.chart.titers().number_of_layers() > 0

    def _format_header(self, name: str, en: dict, header_prefix: str = None):
        return f"{header_prefix or ''}{name}"

    def __bool__(self):
        return bool(self.data)

    def report(self, header_prefix: str = None) -> str:
        if not self.data:
            return ""
        return "\n".join(self._format_entry(name, header_prefix) for name in sorted(self.data, key=self._sorting_key))

    def _sorting_key(self, name: str):
        return name

    def _format_entry(self, name: str, header_prefix: str = None):
        en = self.data[name]
        subentries = "\n  ".join(f"{passage[:3]} " + "\n      ".join(self._format_subentry(passage=passage, antigens=en[passage])) for passage in sPassages if passage in en)
        return f"{self._format_header(name=name, en=en, header_prefix=header_prefix)}\n  {subentries}"

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

def attributes(chart: ae_backend.chart_v3.Chart, semantic_key: str, entries: list, make_attribute_value: callable = lambda **args: True):
    """entries are returned by acmacs-data/semantic-serology semantic_data_for_subtype(subtype).
    [{"name": "Wisconsin/588/2019", "passage": "egg|cell|reassortant", **args}]
    """
    result = Result(chart)
    for en in entries:
        _semantic_entry(chart=chart, semantic_key=semantic_key, result=result, make_attribute_value=make_attribute_value, **en)
    return result

# ----------------------------------------------------------------------

def _semantic_entry(chart: ae_backend.chart_v3.Chart, semantic_key: str, name: str, passage: str, make_attribute_value: callable, result: Result = None, **args) -> Result:
    layout = chart.projection().layout() if chart.number_of_projections() else None # avoid disconnected if projection present
    antigen_name = virus.add_subtype_prefix(chart, name.upper())
    if result is None:
        result = Result(chart)
    subreport = {}
    for psg in _passages(passage):
        antigens = chart.select_antigens(lambda ag: ag.name == antigen_name and ag.passage_is(psg) and layout.connected(ag.point_no))
        if len(antigens):
            if result._has_layers and len(antigens) > 1:
                antigens.sort_by_number_of_layers_descending()
            antigens[0][1].semantic.set(semantic_key, make_attribute_value(passage=passage, antigen=antigens[0][1], **args))
            subreport[psg] = antigens
    if subreport:
        result.data[name] = {**subreport, **args}
    return result

# ----------------------------------------------------------------------

def _passages(passage_type: str):
    if passage_type:
        return [passage_type]
    else:
        return sPassages

# ======================================================================

# sModifier = {"outline": "black", "raise": True, "size": 70, "only": "antigens"}
# sLabelModifier = {"offset": [0, 1], "slant": "normal", "weight": "normal", "size": 36.0, "color": "black"}

def style(chart: ae_backend.chart_v3.Chart, style_name: str, semantic_key: str, point_style: dict, label_style: dict) -> set[str]:
    """Add plot style"""
    style = chart.styles()[style_name]
    style.priority = 1000
    style.add_modifier(selector={semantic_key: True}, **point_style)
    # individual point modifiers with label data
    locdb = ae_backend.locdb_v3.locdb()
    for no, antigen in chart.select_antigens(lambda ag: bool(ag.antigen.semantic.get(semantic_key))):
        if reassortant := antigen.reassortant():
            passage_type = f"{reassortant}-egg"
        else:
            passage_type = antigen.passage().passage_type()
        if name_parsing_result := ae_backend.virus.name_parse(antigen.name()):
            name = f"{locdb.abbreviation(name_parsing_result.parts.location)}/{name_parsing_result.parts.year[2:]}"
        else:
            name = antigen.name()
        style.add_modifier(selector={"!i": no}, outline_width=4.0, label={**label_style, "text": f"{name}-{passage_type}"})
        # print(f">>>> {no} {antigen.designation()}", file=sys.stderr)
    return set([name])

# ======================================================================
