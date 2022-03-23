import ae_backend

# ----------------------------------------------------------------------

def name_format(name: str | ae_backend.chart_v3.Antigen | ae_backend.chart_v3.Serum, format: str):
    """To get a list of possible format keywords use name_format(ag|sr|name, "{?}")"""
    if isinstance(name, str):
        parsed = ae_backend.virus.name_parse(name)
        mapping = MappingName(parsed)
    else:
        parsed = ae_backend.virus.name_parse(name.name())
        mapping = MappingAntigenSerum(parsed, name)
    return format.format_map(mapping)

# ----------------------------------------------------------------------

class MappingName (dict):

    def __init__(self, parsed_name: ae_backend.virus.VirusNameParsingResult):
        self.parsed_name = parsed_name

    def __missing__(self, key):
        if key == "?":
            return self._format_keys(self._raw(key))
        elif (value := self._raw(key)) is not None:
            return value
        else:
            return f"{{{key}}}"

    def _raw(self, key):
        if key == "?":
            return [en for en in dir(self.parsed_name.parts) if en[0] != "_"]
        elif (attr := getattr(self.parsed_name.parts, key, None)) is not None:
            if callable(attr):
                return attr()
            else:
                return attr
        else:
            return None

    def _format_keys(self, keys):
        return "{" + "} {".join(sorted(keys)) + "}"

# ----------------------------------------------------------------------

class MappingAntigenSerum (MappingName):

    def __init__(self, parsed_name: ae_backend.virus.VirusNameParsingResult, ag_sr: ae_backend.chart_v3.Antigen | ae_backend.chart_v3.Serum):
        super().__init__(parsed_name)
        self.ag_sr = ag_sr

    def __missing__(self, key):
        if key == "?":
            if isinstance(self.ag_sr, ae_backend.chart_v3.Antigen):
                return self._format_keys(set(super()._raw(key)) | {"designation", "passage", "passage_without_date", "reassortant", "annotations", "date", "lab_id", "aa", "aa-<no>", "laa-<no>", "nuc", "nuc-<no>", "lnuc-<no>"})
            else:
                return self._format_keys(set(super()._raw(key)) | {"designation", "passage", "passage_without_date", "reassortant", "annotations", "serum_id", "serum_species", "aa", "aa-<no>", "laa-<no>", "nuc", "nuc-<no>", "lnuc-<no>"})
        elif (value := super()._raw(key)) is None:
            if key == "aa":
                return str(self.ag_sr.sequence_aa())
            elif key.startswith("aa-"):
                return self.ag_sr.sequence_aa()[int(key[3:])]
            elif key.startswith("laa-"):
                return f"{key[4:]}{self.ag_sr.sequence_aa()[int(key[4:])]}"
            elif key == "nuc":
                return str(self.ag_sr.sequence_nuc())
            elif key.startswith("nuc-"):
                return self.ag_sr.sequence_nuc()[int(key[4:])]
            elif key.startswith("lnuc-"):
                return f"{key[5:]}{self.ag_sr.sequence_aa()[int(key[5:])]}"
            elif key == "passage_without_date" and isinstance(self.ag_sr, ae_backend.chart_v3.Antigen):
                return self.ag_sr.passage().without_date()
            elif (attr := getattr(self.ag_sr, key, None)) is not None:
                value = attr()
                if isinstance(value, list):
                    return " ".join(str(elt) for elt in value)
                else:
                    return value
            else:
                return f"{{{key}}}"
        else:
            return value

# ----------------------------------------------------------------------
