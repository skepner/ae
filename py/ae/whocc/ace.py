import sys, re, pprint
import ae_backend
from locdb_v2 import geonames, geonames_make_eval
from ..utils import json

# ======================================================================

class DataFixer:

    sDistinct = "DISTINCT"

    def __init__(self, ace_data: dict):
        self.ace_data = ace_data
        self.type_subtype = self.ace_data["c"].get("i", {}).get("V", "")
        if not self.type_subtype:
            print(f">> no type_subtype in ace data: {self.ace_data.get('i')}", file=sys.stderr)
        self.report_data = []
        self.not_found_locations = set()

    def process(self, report: bool = True):
        self.antigen_names()
        self.serum_names()
        self.antigen_passages()
        self.serum_passages()
        self.antigen_dates()
        self.mark_duplicates_as_distinct()
        self.detect_reference_antigens() # after mark_duplicates_as_distinct!
        if report:
            self.report()

    # ----------------------------------------------------------------------

    def antigen_names(self):
        for no, antigen in enumerate(self.ace_data["c"]["a"]):
            self._name(antigen, ag_sr="AG", no=no)

    def serum_names(self):
        for no, serum in enumerate(self.ace_data["c"]["s"]):
            self._name(serum, ag_sr="SR", no=no)

    def antigen_passages(self):
        for no, antigen in enumerate(self.ace_data["c"]["a"]):
            self._passage(antigen, ag_sr="AG", no=no)

    def serum_passages(self):
        for no, serum in enumerate(self.ace_data["c"]["s"]):
            self._passage(serum, ag_sr="SR", no=no)

    def antigen_dates(self):
        pass
        # for no, antigen in enumerate(self.ace_data["c"]["a"]):
        #     if orig_date := antigen.get("D"):
        #         p
        #         if antigen["D"] != orig_date:
        #             self.report_data.append(f"    AG {no:3d} date: \"{antigen['D']}\" <- \"{orig_date}\"")

    def detect_reference_antigens(self):
        "set refernece attribute for all antigens whose names (without reassortant, annotations, passage) are the same as sera names. call after mark_duplicates_as_distinct!"
        serum_names = set(serum["N"] for serum in self.ace_data["c"]["s"])
        for no, antigen in enumerate(self.ace_data["c"]["a"]):
            if "R" in antigen.get("S", ""):
                antigen["S"] = antigen["S"].replace("R", "") # remove old reference attribute
            if self.sDistinct not in antigen.get("a", []) and antigen["N"] in serum_names:
                antigen.setdefault("S", "")
                antigen["S"] += "R"

    def mark_duplicates_as_distinct(self):
        full_names = {}
        for no, antigen in enumerate(self.ace_data["c"]["a"]):
            full_name = " ".join(str(part) for part in (antigen.get(part_name) for part_name in ["N", "R", "a", "P"]) if part)
            full_names.setdefault(full_name, []).append(no)
        for full_name, nos in full_names.items():
            if len(nos) > 1:
                for no in nos[1:]:
                    annotations = self.ace_data["c"]["a"][no].get("a", [])
                    if self.sDistinct not in annotations:
                        annotations.append(self.sDistinct)
                        self.ace_data["c"]["a"][no]["a"] = annotations
                        self.report_data.append(f">>  AG {no:3d} distinct \"{full_name}\", see AG {nos[0]}")

    def report(self):
        if self.report_data or self.not_found_locations:
            print()
        if self.report_data:
            print(f">>> Messages ({len(self.report_data)}):")
            print("\n".join(self.report_data))
            print()
        if self.not_found_locations:
            print(f">>> Unrecognized locations ({len(self.not_found_locations)}):")
            print("    ", "\n    ".join(sorted(self.not_found_locations)), sep="")
            print()
            for name in sorted(self.not_found_locations):
                if gnm := geonames(name=name):
                    print(json.dumps(geonames_make_eval(look_for=name, entries=gnm), indent=2))
                    # for ge in gnm:
                    #     print(f">>>> {ge}")
                else:
                    print(f">> not in geonames: \"{name}\"", file=sys.stderr)
            print()

    # ----------------------------------------------------------------------

    def _name(self, entry: dict, ag_sr: str, no: int):
        parsing_result = ae_backend.virus.name_parse(entry["N"], type_subtype=self.type_subtype)
        # print(f">>>> parsing \"{entry['N']}\": {parsing_result.good()}", file=sys.stderr)
        if parsing_result.good():
            name_parts = parsing_result.parts
            if self.type_subtype:
                name = f"{self.type_subtype}/{name_parts.host_location_isolation_year()}"
            else:
                name = name_parts.host_location_isolation_year()
            if name != entry["N"]:
                self.report_data.append(f"    {ag_sr} {no:3d} name: \"{name}\" <- \"{entry['N']}\"")
                entry["N"] = name
            if reassortant := name_parts.reassortant:
                if not entry.get("R"):
                    entry["R"] = reassortant
                    self.report_data.append(f"    {ag_sr} {no:3d} reassortant: \"{reassortant}\"")
                else:
                    entry["R"] = " ".join([entry["R"], reassortant])
                    self.report_data.append(f"    {ag_sr} {no:3d} addtional reassortant: \"{reassortant}\"")
            if extra := name_parts.extra:
                entry["a"] = entry.get("a", [])
                entry["a"].append(extra)
                self.report_data.append(f">>  {ag_sr} {no:3d} extra: \"{entry['a']}\"")
        else:
            try:
                messages = [f"{msg.type}: {msg.value}" for msg in parsing_result.messages]
            except UnicodeDecodeError as err:
                print(f">> parsing_result.messages {parsing_result.messages}", file=sys.stderr)
                messages = [f"{msg.type}: {err}" for msg in parsing_result.messages]
                print(f">> {ag_sr} {no}: {messages}", file=sys.stderr)
            self.report_data.append(f">>  {ag_sr} {no:3d} name parsing failed \"{entry['N']}\": {messages}")
            # print(f">> name parsing failed: \"{entry['N']}\":",
            #       "\n    ".join(f"{msg.type}: {msg.value}" for msg in parsing_result.messages),
            #       sep="\n    ", file=sys.stderr)
            try:
                self.not_found_locations |= parsing_result.messages.unrecognized_locations()
            except UnicodeDecodeError as err:
                pass

    def _passage(self, entry: dict, ag_sr: str, no: int):
        if orig_passage := entry["P"]:
            if m1 := re.search(r"\s(10-[0-9]+)$", orig_passage): # concentration in Crick
                entry.setdefault("a", []).append(m1.group(1))
                orig_passage = orig_passage[:m1.start()]
            parsing_result = ae_backend.virus.passage_parse(orig_passage)
            self.report_data.append(f">> passage_parse    {ag_sr} {no:3d} passage: {parsing_result.passage()} <- \"{orig_passage}\"")
            if parsing_result.good():
                entry["P"] = parsing_result.passage()
                if entry["P"] != orig_passage:
                    self.report_data.append(f"    {ag_sr} {no:3d} passage: \"{entry['P']}\" <- \"{orig_passage}\"")
            else:
                messages = [f"{msg.type}: {msg.value}" for msg in parsing_result.messages]
                self.report_data.append(f">>  {ag_sr} {no:3d} passage parsing failed \"{orig_passage}\": {messages}")

# ======================================================================
