from pathlib import Path
import ae_backend

# ======================================================================

class Error (RuntimeError): pass

# ----------------------------------------------------------------------

class reader:

    def __init__(self, filename: Path):
        self.reader_ = ae_backend.FastaReader(filename)
        self.messages_ = []

    def __iter__(self):
        for en in self.reader_:
            metadata = gisaid_name_parser(en.name, messages=self.messages_) or regular_name_parser(en.name)
            # fix metadata["name"]
            yield metadata, en.sequence

# ----------------------------------------------------------------------

def regular_name_parser(name: str):
    return {"name": name}

# ----------------------------------------------------------------------

def gisaid_name_parser(name: str, messages: list) -> str:
    fields = name.split("_|_")
    if len(fields) == 1:
        return None             # not a gisaid
    elif len(fields) == 18 and fields[-1] == "":
        # print("  {}".format('\n  '.join(fields)))
        return dict(gisaid_parse_fields(fields, messages=messages)) # {"name": fields[0], **{sGisaidFieldKeys[key]: value for key, value in (fn.split("=", maxsplit=1) for fn in fields[1:]) if value}}
    else:
        raise Error(f"Invalid number of fields in the gisaid-like name: {len(fields)}: \"{name}\"")

# ----------------------------------------------------------------------

def gisaid_parse_fields(fields, messages):
    yield "name", fields[0]
    for field in fields[1:-1]:
        key, value = field.split("=", maxsplit=1)
        value = value.strip()
        if value:
            field_name, parser = sGisaidFieldKeys[key]
            if parser:
                yield field_name, parser(value, messages=messages)
            else:
                yield field_name, value

def gisaid_parse_subtype(subtype, messages):
    return subtype.upper()

# def parse_lineage(lineage, messages):
#     return lineage

def parse_date(date, messages):
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
