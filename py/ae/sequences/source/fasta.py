import sys, re, pprint
from pathlib import Path

import ae_backend
from .parse import Context, parse_name, parse_date

# ======================================================================

class Error (RuntimeError): pass

# ----------------------------------------------------------------------

class reader:

    def __init__(self, filename: Path):
        self.reader_ = ae_backend.raw_sequence.FastaReader(filename)
        self.messages = []
        self.unrecognized_locations = set()
        if filename.name[:5] == "niid-":
            self.lab_hint = "NIID"
        else:
            self.lab_hint = None

    def __iter__(self):
        for en in self.reader_:
            context = Context(self, filename=Path(en.filename), line_no=en.line_no)
            metadata = \
                gisaid_name_parser(en.raw_name, context=context) \
                or naomi_name_parser(en.raw_name, context=context) \
                or regular_name_parser(en.raw_name, lab_hint=self.lab_hint, context=context)
            yield metadata, en.sequence # metadata may contain "excluded" key to manually exclude the sequence

# ----------------------------------------------------------------------

sRePassageAtEnd = re.compile(r"^(.+)_(E|EGG|CELL|SIAT|MDCK|OR)$", re.I)

def regular_name_parser(name: str, lab_hint: str, context: Context):
    # print(f">>> regular_name_parser \"{name}\"")
    metadata = {"name": name}
    if lab_hint:
        metadata["lab"] = lab_hint
        if lab_hint == "NIID" and (mm := sRePassageAtEnd.match(name)):
            metadata["passage"] = mm.group(2)
            if metadata["passage"] in ["E", "MDCK", "SIAT"]:
               metadata["passage"] += "?"
            elif metadata["passage"] == "EGG":
                metadata["passage"] = "E?"
            elif metadata["passage"] == "CELL":
                metadata["passage"] = "MDCK?"
            name = mm.group(1)
    metadata["name"] = parse_name(name, metadata=metadata, context=context)
    return metadata

# ======================================================================
# gisaid
# ======================================================================

def gisaid_name_parser(name: str, context: Context) -> str:
    fields = [en.strip() for en in name.split("_|_")]
    if len(fields) == 1:
        return None             # not a gisaid
    elif len(fields) == 18 and fields[-1] == "":
        # print("  {}".format('\n  '.join(fields)))
        return gisaid_extract_fields(fields, context=context)
    elif len(fields) == 19 and fields[-1] == "" and fields[-2].startswith("x="):
        # manually excluded
        return {"excluded": fields[-2][2:], **gisaid_extract_fields(fields[:-2] + [""], context=context)}
    else:
        raise Error(f"Invalid number of fields in the gisaid-like name: {len(fields)}: \"{name}\"")

# ----------------------------------------------------------------------

def gisaid_extract_fields(fields: list, context: Context):
    metadata = {"name": fields[0]}
    for field in fields[1:-1]:
        key, value = field.split("=", maxsplit=1)
        if value:
            metadata[sGisaidFieldKeys[key]] = value.strip()
    for field_name, parser in sGisaidFieldParsers:
        if field_value := metadata.get(field_name):
            metadata[field_name] = parser(field_value, metadata=metadata, context=context)
    return metadata

def gisaid_parse_subtype(subtype: str, metadata: dict, context: Context):
    subtype = subtype.upper()
    if len(subtype) >= 8 and subtype[0] == "A":
        if subtype[5] != "0" and subtype[7] == "0": # H3N0
            return f"A({subtype[4:6]})"
        else:
            return f"A({subtype[4:]})"
    elif len(subtype) > 0 and subtype[0] == "B":
        return "B"
    else:
        context.message(field="type_subtype", value=subtype, message=f"[gisaid]: invalid subtype")
        return ""

def parse_lineage(lineage, metadata: dict, context: Context):
    return lineage.upper()

def gisaid_parse_lab(lab: str, metadata: dict, context: Context):
    return sGisaidLabs.get(lab.upper(), lab)

sGisaidFieldKeys = {
    "a": "isolate_id",
    "b": "type_subtype",
    "c": "passage",
    "d": "lineage",
    "e": "date",
    "f": "submitter",
    "g": "sample_id_by_sample_provider",
    "h": "lab_id",
    "i": "gisaid_last_modified",
    "j": "originating_lab",
    "k": "lab",
    "l": "gisaid_segment",
    "m": "gisaid_segment_number",
    "n": "gisaid_identifier",
    "o": "gisaid_dna_accession_no",
    "p": "gisaid_dna_insdc",
}

sGisaidFieldParsers = [
    ["type_subtype",                  gisaid_parse_subtype],
    [ "lineage",                      parse_lineage],
    ["date",                          parse_date],
    ["lab",                           gisaid_parse_lab],
    ["gisaid_last_modified",          parse_date],

    ["name",                          parse_name] # name must be the last!
]

sGisaidLabs = {
    "CENTERS FOR DISEASE CONTROL AND PREVENTION": "CDC",
    "WHO CHINESE NATIONAL INFLUENZA CENTER": "CNIC",
    "CRICK WORLDWIDE INFLUENZA CENTRE": "CRICK",
    "NATIONAL INSTITUTE FOR MEDICAL RESEARCH": "Crick",
    "NATIONAL INSTITUTE OF INFECTIOUS DISEASES (NIID)": "NIID",
    "WHO COLLABORATING CENTRE FOR REFERENCE AND RESEARCH ON INFLUENZA": "VIDRL",
    "ERASMUS MEDICAL CENTER": "EMC",
    "NATIONAL INSTITUTE FOR BIOLOGICAL STANDARDS AND CONTROL (NIBSC)": "NIBSC",
    }

# ======================================================================
# naomi
# ======================================================================

def naomi_name_parser(name: str, context: Context) -> str:
    fields = [en.strip() for en in name.split(" | ")]
    if len(fields) == 1:
        return None             # not a naomi
    elif len(fields) == 7: # name, date, passage, <unknown>, lab, subtype, pdm09
        return naomi_extract_fields(fields, context=context)
    else:
        raise Error(f"Invalid number of fields in the gisaid-like name: {len(fields)}: \"{name}\"")

# ----------------------------------------------------------------------

def naomi_extract_fields(fields: list, context: Context):
    metadata = {
        "name": fields[0].strip(),
        "date": fields[1].strip(),
        "passage": fields[2].strip(),
        # unknown (lineage?)
        "lab": fields[4].strip(),
        "type_subtype": fields[5].strip(),
        # pdm09
    }
    for field_name, parser in sGisaidFieldParsers:
        if field_value := metadata.get(field_name):
            metadata[field_name] = parser(field_value, metadata=metadata, context=context)
    return metadata

# ======================================================================

def add_metadata_to_sequence(metadata: dict, sequence: ae_backend.raw_sequence.Sequence):
    sequence.name = metadata["name"]
    if accession_number := (metadata.get("isolate_id") or metadata.get("sample_id_by_sample_provider")):
        sequence.accession_number = accession_number
    if date := metadata.get("date"):
        sequence.date = date
    if type_subtype := metadata.get("type_subtype"):
        sequence.type_subtype = type_subtype
    if lab := metadata.get("lab"):
        sequence.lab = lab
    if lab_id := metadata.get("lab_id"):
        sequence.lab_id = lab_id
    if lineage := metadata.get("lineage"):
        sequence.lineage = lineage
    if passage := metadata.get("passage"):
        sequence.passage = passage
    if gisaid_dna_accession_no := metadata.get("gisaid_dna_accession_no"):
        sequence.gisaid_dna_accession_no = gisaid_dna_accession_no
    if gisaid_dna_insdc := metadata.get("gisaid_dna_insdc"):
        sequence.gisaid_dna_insdc = gisaid_dna_insdc
    if gisaid_identifier := metadata.get("gisaid_identifier"):
        sequence.gisaid_identifier = gisaid_identifier
    if gisaid_last_modified := metadata.get("gisaid_last_modified"):
        sequence.gisaid_last_modified = gisaid_last_modified

# ======================================================================
