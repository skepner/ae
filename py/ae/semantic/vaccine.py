import sys, pprint, json
from typing import Optional
import ae_backend
from .find import AntigenFinder, PASSAGES
# from .. import virus
from .name_generator import NameGenerator

# ======================================================================

class Vaccine:

    class Entry:

        def __init__(self, no: int, antigen: ae_backend.chart_v3.Antigen, layers: list):
            self.no = no
            self.antigen = antigen
            self.layers = layers

        def __str__(self):
            return f"{self.no:4d} {self.antigen.designation()} (layers: {len(self.layers)})"

        def to_dict(self):
            return {"no": self.no, "designation": self.antigen.designation(), "layers": self.layers}

    # ----------------------------------------------------------------------

    def __init__(self, name: str, year: str, surrogate: bool):
        self.name = name
        self.year = year
        self.surrogate = surrogate
        self.cell: Optional[list[Vaccine.Entry]] = None
        self.egg: Optional[list[Vaccine.Entry]] = None
        self.reassortant: Optional[list[Vaccine.Entry]] = None
        for passage in PASSAGES:
            setattr(self, passage, [])

    def __bool__(self):
        return any(bool(getattr(self, passage, None)) for passage in PASSAGES)

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
        for passage_type in PASSAGES:
            for no, en in enumerate(getattr(self, passage_type, [])):
                if no == 0:
                    prefix = f"{passage_type[:4]:<4s}"
                else:
                    prefix = " " * 4
                print(f"  {prefix}  {en}", file=sys.stderr)
        print(file=sys.stderr)

    @classmethod
    def make(cls, chart: ae_backend.chart_v3.Chart, finder: AntigenFinder, name: str, year: str = None, passage: str = None, surrogate: bool = False, **ignored):
        vaccine = Vaccine(name=name, year=year, surrogate=surrogate)
        for psg, found in finder.find(name=name, passage=passage).items():
            setattr(vaccine, psg, [cls.Entry(**en) for en in found])
        return vaccine

# ----------------------------------------------------------------------

def find(chart: ae_backend.chart_v3.Chart, semantic_attribute_data: list, report: bool = True) -> list[Vaccine]:
    """Return list of vaccine entries (one entry per name).
    semantic_attribute_data is loaded from e.g. acmacs-data/semantic-vaccines.py
    order of returned elements is the same as entries
    """
    finder = AntigenFinder(chart)
    data = [en for en in (Vaccine.make(chart, finder, **source) for source in semantic_attribute_data) if en]
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
        # "size": 70.0,
        # "outline_width": 1.0,
        # "label_size": 36.0,
        "label": name_generator.location_year2_passaga_type(antigen),
        "semantic": antigen.semantic.get("V")
    } for no, antigen in chart.select_antigens(lambda ag: bool(ag.antigen.semantic.get("V")))]
    vaccine_data.sort(key=lambda en: en["semantic"] + en["designation"])
    # pprint.pprint(vaccine_data, width=200)
    return vaccine_data

# ----------------------------------------------------------------------

def default_field_order():
    return ["no", "designation", "lox", "loy", "fill", "label", "semantic", "label_size", "size", "outline_width"]

# ----------------------------------------------------------------------

def update(collected: list[dict[str, object]], user: list[dict[str, object|str]], match_by: str = "designation") -> list[dict[str, object]]:
    """match_by is a unique key! user data order is preferred, extra collected data is at the beginning of the result"""
    collected_ref = {en[match_by]: en for en in collected}
    user_ref = {en[match_by]: en for en in user}

    def upd(src: Optional[dict[str, object]], upd: dict[str, object]) -> Optional[dict[str, object]]:
        if not src:
            return None
        res = {**src}
        for key, val in upd.items():
            if key == "no":
                if str(val) != str(res[key]):
                    print(f">> vaccine.update: no mismatch (\"{val}\" vs. \"{res[key]}\") for matching key: {src} -- {upd}", file=sys.stderr)
            elif key != match_by and val is not None and val != "":
                res[key] = val
        return res

    user_result = [user_updated for user_updated in (upd(collected_ref.get(usr[match_by]), usr) for usr in user) if user_updated]
    collected_not_in_user = [coll for coll in collected if coll[match_by] not in user_ref]
    return collected_not_in_user + user_result

# ======================================================================

def style(chart: ae_backend.chart_v3.Chart, style_name: str, data: list[dict[str, object]], common_modifier: dict, label_modifier: dict, priority: int = 1000) -> set[str]:
    """Add "-vaccines" and "-vaccines-blue" plot style
    data is the output of find() and/or update()
    common_modifier: {"outline": "black", "fill": ":bright", "size": 50, "outline_width": 1.0}
    label_modifier: {"size": 30.0, "slant": "normal", "weight": "normal", "color": "black"} - other fields are taken from data
    """

    style = chart.styles()[style_name]
    style.priority = 1000
    style.add_modifier(selector={"V": True}, only="antigens", raise_=True, **common_modifier)

    for en in data:
        style.add_modifier(selector={"!i": en["no"]}, only="antigens", **extract_point_modifier_data(source=en, label_modifier=label_modifier))
    return {style_name}

# ----------------------------------------------------------------------

def extract_point_modifier_data(source: dict[str, object], label_modifier: dict) -> dict[str, object]:
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

# ----------------------------------------------------------------------
