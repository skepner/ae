import pprint
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

def vaccine(chart: ae_backend.chart_v3.Chart, name: str, year: str = None, passage: str = None, surrogate: bool = False, result: Result = None, **ignored) -> Result:
    """Add "V" semantic attribute to the vaccine antigens.
    passage: "egg", "cell", "reassortant"
    """
    # print(f"> {name} {passage} [{year}]")
    layout = chart.projection().layout() if chart.number_of_projections() else None
    vac_name = virus.add_subtype_prefix(chart, name)
    if result is None:
        result = Result(chart)
    subreport = {}
    for psg in (sPassages if passage is None else [passage]):
        antigens = chart.select_antigens(lambda ag: ag.name == vac_name and ag.passage_is(psg) and layout.connected(ag.point_no))
        if len(antigens):
            if result._has_layers and len(antigens) > 1:
                antigens.sort_by_number_of_layers_descending()
            antigens[0][1].semantic.vaccine((year or "") + ("s" if surrogate else ""))
            subreport[psg] = antigens
    if subreport:
        result.data[name] = {"year": year, "surrogate": surrogate, **subreport}
    return result

# ----------------------------------------------------------------------

def vaccines(chart: ae_backend.chart_v3.Chart, entries: list) -> Result:
    result = Result(chart)
    for en in entries:
        vaccine(chart=chart, result=result, **en)
    return result

# ----------------------------------------------------------------------

def vaccines_and_plot_style(chart: ae_backend.chart_v3.Chart, entries: list, modifier: dict = {"outline": "black", "rais": True, "size": 70, "only": "antigens"}, report: bool = True) -> str:
    if result := vaccines(chart=chart, entries=entries):
        style_name = "-vaccines"
        style = chart.styles()[style_name]
        style.priority = 1000
        style.add_modifier(selector=["V", ""], **modifier)
        if report:
            print(result.report())
        return style_name
    else:
        return None

# ======================================================================
