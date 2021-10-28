from pathlib import Path
from dataclasses import dataclass
from typing import Callable

import ae_backend

# ======================================================================

class Error (RuntimeError): pass

# ----------------------------------------------------------------------

class reader:

    @dataclass
    class Message:
        message: str
        filename: str
        line_no: int

    def __init__(self, filename: Path):
        self.reader_ = ae_backend.FastaReader(filename)
        self.messages = []

    def __iter__(self):
        for en in self.reader_:
            metadata = gisaid_name_parser(en.name, make_message=self.message_maker(filename=en.filename, line_no=en.line_no)) or regular_name_parser(en.name)
            # fix metadata["name"]
            yield metadata, en.sequence

    def message_maker(self, filename: str, line_no: int):
        def make_message(msg):
            self.messages.append(Message(msg, filename=filename, line_no=line_no))
        return make_message

# ----------------------------------------------------------------------

def regular_name_parser(name: str):
    return {"name": name}

# ----------------------------------------------------------------------

def parse_name(name: str, metadata: dict, make_message: Callable):
    return name

# ----------------------------------------------------------------------

def parse_date(date: str, metadata: dict, make_message: Callable):
    try:
        return ae_backend.date_format(date, throw_on_error=True)
    except Exception as err:
        make_message(str(err))
        return date

# ======================================================================
# gisaid
# ======================================================================

def gisaid_name_parser(name: str, make_message: Callable) -> str:
    fields = name.split("_|_")
    if len(fields) == 1:
        return None             # not a gisaid
    elif len(fields) == 18 and fields[-1] == "":
        # print("  {}".format('\n  '.join(fields)))
        return gisaid_extract_fields(fields, make_message=make_message)
    else:
        raise Error(f"Invalid number of fields in the gisaid-like name: {len(fields)}: \"{name}\"")

# ----------------------------------------------------------------------

def gisaid_extract_fields(fields: list, make_message: Callable):
    metadata = {"name": fields[0]}
    for field in fields[1:-1]:
        key, value = field.split("=", maxsplit=1)
        if value:
            metadata[sGisaidFieldKeys[key]] = value.strip()
    for field_name, parser in sGisaidFieldParsers.items():
        if field_value := metadata.get(field_name):
            metadata[field_name] = parser(field_value, metadata=metadata, make_message=make_message)
    return metadata

def gisaid_parse_subtype(subtype: str, metadata: dict, make_message: Callable):
    subtype = subtype.upper()
    if len(subtype) >= 8 and subtype[0] == "A":
        if subtype[5] != "0" and subtype[7] == "0": # H3N0
            return f"A({subtype[4:6]})"
        else:
            return f"A({subtype[4:]})"
    elif len(subtype) > 0 and subtype[0] == "B":
        return "B"
    else:
        make_message(f"[gisaid]: invalid subtype: {subtype}")
        return ""

# def parse_lineage(lineage, metadata: dict, make_message: Callable):
#     return lineage

def gisaid_parse_lab(lab: str, metadata: dict, make_message: Callable):
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

sGisaidFieldParsers = {
    "name":                          parse_name,
    "type_subtype":                  gisaid_parse_subtype,
    # "lineage":                      parse_lineage,
    "date":                          parse_date,
    "lab":                           gisaid_parse_lab,
    "gisaid_last_modified":          parse_date,
}

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

# ----------------------------------------------------------------------
