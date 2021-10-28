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
        return dict(gisaid_parse_fields(fields, make_message=make_message))
    else:
        raise Error(f"Invalid number of fields in the gisaid-like name: {len(fields)}: \"{name}\"")

# ----------------------------------------------------------------------

def gisaid_parse_fields(fields, make_message: Callable):
    yield "name", fields[0]
    for field in fields[1:-1]:
        key, value = field.split("=", maxsplit=1)
        value = value.strip()
        if value:
            field_name, parser = sGisaidFieldKeys[key]
            if parser:
                yield field_name, parser(value, make_message=make_message)
            else:
                yield field_name, value

def gisaid_parse_subtype(subtype, make_message: Callable):
    return subtype.upper()

# def parse_lineage(lineage, make_message: Callable):
#     return lineage

def parse_date(date, make_message: Callable):
    return ae_backend.date_format(date)

sGisaidFieldKeys = {
    "a": ["isolate_id",                    None],
    "b": ["type_subtype",                  gisaid_parse_subtype],
    "c": ["passage",                       None],
    "d": ["lineage",                       None], # parse_lineage],
    "e": ["date",                          parse_date],
    "f": ["submitter",                     None],
    "g": ["sample_id_by_sample_provider",  None],
    "h": ["lab_id",                        None],
    "i": ["gisaid_last_modified",          parse_date],
    "j": ["originating_lab",               None],
    "k": ["lab",                           None],
    "l": ["gisaid_segment",                None],
    "m": ["gisaid_segment_number",         None],
    "n": ["gisaid_identifier",             None],
    "o": ["gisaid_dna_accession_no",       None],
    "p": ["gisaid_dna_insdc",              None]
}

# ----------------------------------------------------------------------
