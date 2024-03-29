import sys, pprint, json
from typing import Optional
import ae_backend.chart_v3
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

    def __init__(self, name: str, year: Optional[str], surrogate: bool):
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
        val = f"{self.year or ''}{passage[0]}{'s' if self.surrogate else ''}"
        if self.year in current_vaccine_years:
            val += "C"
        print(f">>>> semantic_vaccine {entry.antigen.designation()} \"{val}\"", file=sys.stderr)
        entry.antigen.semantic.vaccine(val)

    def __repr__(self):
        return (json.dumps({"name": self.name, "year": self.year, "surrogate": self.surrogate, "cell": [en.to_dict() for en in self.cell], "egg": [en.to_dict() for en in self.egg], "reassortant": [en.to_dict() for en in self.reassortant]}))

    def report(self) -> list[str]:
        result = [f"{self.name} [{self.year}]{' <surrogate>' if self.surrogate else ''}"]
        for passage_type in PASSAGES:
            for no, en in enumerate(getattr(self, passage_type, [])):
                if no == 0:
                    prefix = f"{passage_type[:4]:<4s}"
                else:
                    prefix = " " * 4
                result.append(f"  {prefix}  {en}")
        return result

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
        print("\n".join(get_report(data)), file=sys.stderr)
    return data

def report(vaccines_found: list[Vaccine]) -> list[str]:
    result: list[str] = []
    for en in vaccines_found:
        if result:
            result.append("")
        result.extend(en.report())
    return result

get_report = report           # alias to avoid renaming report argument in find()

# ----------------------------------------------------------------------

def set_semantic(vaccines_found: list[Vaccine], current_vaccine_years: list[str] = [], disable: dict[str, dict[str, list[str]]] = {}, choose: dict[str, list[dict[str, str|int]]] = {}):
    """
    disable: {"any": {"name": ["SOUTH AUSTRALIA/34/2019"]}, "egg": {"name": ["CAMBODIA/E0826360/2020"]}} use "name" or "year" as a selector
    choose: {"egg": [{"name": "VICTORIA/2570/2019", "index": 1}]} use "name" or "year" as a selector to choose index (default is 0) to get from list for passage
    """

    def is_disbaled(vac: Vaccine, selector: dict[str, list[str]]) -> bool:
        return any(getattr(vac, attr_name, None) in vals for attr_name, vals in selector.items())

    def get_index(vac: Vaccine, selector: list[dict[str, str|int]]) -> int:
        # print(f">>>> get_index {vac}", file=sys.stderr)
        for sel in selector:
            if any(getattr(vac, attr_name, None) == val for attr_name, val in sel.items() if attr_name != "index"):
                return sel["index"]
        return 0

    for vaccine in vaccines_found:
        if not is_disbaled(vaccine, disable.get("any", {})):
            print(f">>>> V {vaccine}", file=sys.stderr)
            for passage in ["cell", "egg", "reassortant"]:
                if (vaccines_per_passage := getattr(vaccine, passage)) and not is_disbaled(vaccine, disable.get(passage, {})):
                    if (ind := get_index(vaccine, choose.get(passage, []))) < len(vaccines_per_passage):
                        vaccine.semantic_vaccine(vaccines_per_passage[ind], current_vaccine_years=current_vaccine_years)

# ======================================================================

def collect_data_for_styles(chart: ae_backend.chart_v3.Chart):
    """Look for "V" semantic attribute to collect data for styles"""
    name_generator = NameGenerator()
    vaccine_data: list[dict[str,str|float]] = [{
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
    return ["no", "designation", "lox", "loy", "fill", "fill_v1", "fill_v2", "fill_ts", "label", "semantic", "label_size", "size", "outline_width"]

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

def style(chart: ae_backend.chart_v3.Chart, style_name: str, data: list[dict[str, object]], common_modifier: dict, label_modifier: dict, data_key_mapping: dict[str, str] = {}, priority: int = 1000) -> set[str]:
    """Add "-vaccines" (or "-vaccines-ts") plot style
    data is the output of find() and/or update()
    common_modifier: {"outline": "black", "fill": ":bright", "size": 50, "outline_width": 1.0}
    label_modifier: {"size": 30.0, "slant": "normal", "weight": "normal", "color": "black"} - other fields are taken from data
    data_key_mapping: to support chosing between different fill colors for ts and clades: {"fill": "fill_ts"}
    """

    style = chart.styles()[style_name]
    style.priority = priority
    style.add_modifier(selector={"V": True}, only="antigens", raise_=True, **common_modifier)

    for en in data:
        style.add_modifier(selector={"!i": en["no"]}, only="antigens", show=True, **extract_point_modifier_data(source=en, data_key_mapping=data_key_mapping, label_modifier=label_modifier))
    return {style_name}

# ----------------------------------------------------------------------

def extract_point_modifier_data(source: dict[str, object], data_key_mapping: dict[str, str], label_modifier: dict) -> dict[str, object]:
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
        "fill": source.get(data_key_mapping.get("fill", "fill")),
        "outline": source.get(data_key_mapping.get("outline", "outline")),
        "outline_width": get_float(data_key_mapping.get("outline_width", "outline_width")),
        "size": get_float(data_key_mapping.get("size", "size")),
        "rotation": get_float(data_key_mapping.get("rotation", "rotation")),
        "aspect": get_float(data_key_mapping.get("aspect", "aspect")),
        "shape": source.get(data_key_mapping.get("shape", "shape")),
        "hide": get_bool(data_key_mapping.get("hide", "hide")),
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
