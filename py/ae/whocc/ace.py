import sys
import ae_backend

# ======================================================================

class DataFixer:

    def __init__(self, ace_data: dict):
        self.ace_data = ace_data
        self.report_data = []
        self.not_found_locations = set()
        self.type_subtype = self.ace_data["c"].get("i", {}).get("V", "")
        if not self.type_subtype:
            print(f">> no type_subtype in ace data: {self.ace_data.get('i')}", file=sys.stderr)

    def process(self, report: bool = True):
        self.antigen_names()
        self.serum_names()
        self.antigen_passages()
        self.serum_passages()
        self.mark_duplicates_as_distinct()
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

    def mark_duplicates_as_distinct(self):
        pass

    def report(self):
        # report not found locations
        if self.report_data:
            print(f">>> Messages ({len(self.report_data)}):")
            print("\n".join(self.report_data))
        if self.not_found_locations:
            print(f">>> Unrecognized locations ({len(self.not_found_locations)}):")
            print("    ", "\n    ".join(sorted(self.not_found_locations)), sep="")

    # ----------------------------------------------------------------------

    def _name(self, entry: dict, ag_sr: str, no: int):
        parsing_result = ae_backend.virus_name_parse(entry["N"], type_subtype=self.type_subtype)
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
            messages = [f"{msg.type}: {msg.value}" for msg in parsing_result.messages]
            self.report_data.append(f">>  {ag_sr} {no:3d} name parsing failed \"{entry['N']}\": {messages}")
            # print(f">> name parsing failed: \"{entry['N']}\":",
            #       "\n    ".join(f"{msg.type}: {msg.value}" for msg in parsing_result.messages),
            #       sep="\n    ", file=sys.stderr)
            self.not_found_locations |= parsing_result.messages.unrecognized_locations()

    def _passage(self, entry: dict, ag_sr: str, no: int):
        pass

# ======================================================================
