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

def gisaid_extract_fields(fields, make_message: Callable):
    metadata = {"name": fields[0]}
    for field in fields[1:-1]:
        key, value = field.split("=", maxsplit=1)
        if value:
            metadata[sGisaidFieldKeys[key]] = value.strip()
    for field_name, parser in sGisaidFieldParsers.items():
        if field_value := metadata.get(field_name):
            metadata[field_name] = parser(field_value, metadata=metadata, make_message=make_message)
    return metadata

def gisaid_parse_subtype(subtype, metadata: dict, make_message: Callable):
    return subtype.upper()

# def parse_lineage(lineage, metadata: dict, make_message: Callable):
#     return lineage

def parse_date(date, metadata: dict, make_message: Callable):
    try:
        return ae_backend.date_format(date, throw_on_error=True)
    except Exception as err:
        make_message(str(err))
        return date

def gisaid_parse_lab(lab, metadata: dict, make_message: Callable):
    return sGisaidLabs.get(lab, lab)

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
    "type_subtype":                  gisaid_parse_subtype,
    # "lineage":                      parse_lineage,
    "date":                          parse_date,
    "lab":                           gisaid_parse_lab,
    "gisaid_last_modified":          parse_date,
}

sGisaidLabs = {
    "Centers for Disease Control and Prevention": "CDC",
    "WHO Chinese National Influenza Center": "CNIC",
    "Crick Worldwide Influenza Centre": "Crick",
    "National Institute of Infectious Diseases (NIID)": "NIID",
    "WHO Collaborating Centre for Reference and Research on Influenza": "VIDRL",
    }

# ----------------------------------------------------------------------
