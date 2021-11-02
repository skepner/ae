import pprint
from pathlib import Path
from dataclasses import dataclass
from typing import Callable

import ae_backend
import ae.utils.directory_module

# ======================================================================

class Error (RuntimeError): pass

# ----------------------------------------------------------------------

class reader:

    @dataclass
    class Message:
        field: str
        value: str
        message: str
        filename: Path
        line_no: int

    # ----------------------------------------------------------------------

    class Context:

        def __init__(self, reader, filename: Path, line_no: int):
            self.reader = reader
            self.filename = filename
            self.line_no = line_no

        def message(self, field, value, message):
            self.reader.messages.append(reader.Message(field=field, value=value, message=message, filename=self.filename, line_no=self.line_no))

        def unrecognized_locations(self, unrecognized_locations: set):
            self.reader.unrecognized_locations |= unrecognized_locations

        def preprocess_virus_name(self, name, metadata: dict):
            if (directory_module := ae.utils.directory_module.load(self.filename.parent)) and (preprocessor := getattr(directory_module, "preprocess_virus_name", None)):
                return preprocessor(name, metadata)
            else:
                return name

        def preprocess_date(self, date, metadata: dict):
            if (directory_module := ae.utils.directory_module.load(self.filename.parent)) and (preprocessor := getattr(directory_module, "preprocess_date", None)):
                return preprocessor(date, metadata)
            else:
                return date

    # ----------------------------------------------------------------------

    def __init__(self, filename: Path):
        self.reader_ = ae_backend.FastaReader(filename)
        self.messages = []
        self.unrecognized_locations = set()

    def __iter__(self):
        for en in self.reader_:
            context = self.Context(self, filename=Path(en.filename), line_no=en.line_no)
            metadata = gisaid_name_parser(en.name, context=context) \
                or regular_name_parser(en.name, context=context)
            yield metadata, en.sequence

    # def message_maker(self, filename: str, line_no: int):
    #     def make_message(field, value, message):
    #         self.messages.append(self.Message(field=field, value=value, message=message, filename=filename, line_no=line_no))
    #     return make_message

# ----------------------------------------------------------------------

def regular_name_parser(name: str, context: reader.Context):
    return {"name": name}

# ----------------------------------------------------------------------

def parse_name(name: str, metadata: dict, context: reader.Context):
    preprocessed_name = context.preprocess_virus_name(name, metadata)
    result = ae_backend.virus_name_parse(preprocessed_name)
    if result.good():
        new_name = result.parts.name()
        # if "CNIC" in new_name or "IVR" in new_name or "NYMC" in new_name:
        #     print(f"\"{new_name}\" <-- \"{name}\"")
        return new_name
    else:
        if preprocessed_name != name:
            value = f"{preprocessed_name} (original: {name})"
        else:
            value = name
        for message in result.messages:
            context.message(field="name", value=value, message=f"[{message.type}] {message.value} -- {message.context}")
        context.unrecognized_locations(result.messages.unrecognized_locations())
        return name

# ----------------------------------------------------------------------

def parse_date(date: str, metadata: dict, context: reader.Context):
    preprocessed_date = context.preprocess_date(date, metadata)
    try:
        return ae_backend.date_format(preprocessed_date, throw_on_error=True, month_first=metadata.get("lab") == "CDC")
    except Exception as err:
        if date != preprocessed_date:
            value = f"{preprocessed_date} (original: {date})"
        else:
            value = date
        context.message(field="date", value=value, message=str(err))
        return date

# ======================================================================
# gisaid
# ======================================================================

def gisaid_name_parser(name: str, context: reader.Context) -> str:
    fields = [en.strip() for en in name.split("_|_")]
    if len(fields) == 1:
        return None             # not a gisaid
    elif len(fields) == 18 and fields[-1] == "":
        # print("  {}".format('\n  '.join(fields)))
        return gisaid_extract_fields(fields, context=context)
    else:
        raise Error(f"Invalid number of fields in the gisaid-like name: {len(fields)}: \"{name}\"")

# ----------------------------------------------------------------------

def gisaid_extract_fields(fields: list, context: reader.Context):
    metadata = {"name": fields[0]}
    for field in fields[1:-1]:
        key, value = field.split("=", maxsplit=1)
        if value:
            metadata[sGisaidFieldKeys[key]] = value.strip()
    for field_name, parser in sGisaidFieldParsers:
        if field_value := metadata.get(field_name):
            metadata[field_name] = parser(field_value, metadata=metadata, context=context)
    return metadata

def gisaid_parse_subtype(subtype: str, metadata: dict, context: reader.Context):
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

# def parse_lineage(lineage, metadata: dict, context: reader.Context):
#     return lineage

def gisaid_parse_lab(lab: str, metadata: dict, context: reader.Context):
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
    #[ "lineage",                      parse_lineage],
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

# ----------------------------------------------------------------------
