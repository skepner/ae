from typing import Optional
import ae_backend
from .. import virus

# ======================================================================

PASSAGES = ["cell", "egg", "reassortant"]

class AntigenFinder:

    def __init__(self, chart: ae_backend.chart_v3.Chart):
        self.chart = chart
        self.type_subtype = chart.info().type_subtype()
        self.chart_has_layers = self.chart.titers().number_of_layers() > 0
        self.layout = chart.projection().layout() if chart.number_of_projections() else None # avoid disconnected if projection present

    def find(self, name: str, passage: Optional[str] = None) -> dict[str, list[dict[str, object]]]:
        """passage: egg, cell, reassortant
        return {"egg": [{"no": 1, "antigen": ae_backend.chart_v3.Antigen, "layers": [1, 2, 3]}], "cell": [...], "reassortant": [...]}
        if nothing found, return {}
        antigens in lists are sorted by the number of layers descending
        """
        name = virus.add_subtype_prefix(self.type_subtype, name.upper())
        result: dict[str, list[dict[str, object]]] = {}
        for psg in [passage] if passage else PASSAGES:
            antigens = self.chart.select_antigens(lambda ag: ag.name == name and not ag.distinct() and ag.passage_is(psg) and self._is_connected(ag.point_no))
            if len(antigens):
                if self.chart_has_layers and len(antigens) > 1:
                    antigens.sort_by_number_of_layers_descending()
                result[psg] = [{"no": no, "antigen": antigen, "layers": self.chart.titers().layers_with_antigen(no) if self.chart.titers().number_of_layers() > 1 else [0]} for no, antigen in antigens]
        return result

    def _is_connected(self, point_no: int):
        if self.layout:
            return self.layout.connected(point_no)
        else:
            return True

# ======================================================================
