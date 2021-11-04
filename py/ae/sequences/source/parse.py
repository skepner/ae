import sys
from pathlib import Path
from dataclasses import dataclass

import ae.utils.directory_module
import ae_backend

# ======================================================================

@dataclass
class Message:
    field: str
    value: str
    message: str
    message_raw: ae_backend.Message
    filename: Path
    line_no: int

# ======================================================================

class Context:

    def __init__(self, reader, filename: Path, line_no: int):
        self.reader = reader
        self.filename = filename
        self.line_no = line_no

    def message(self, field, value, message, message_raw=None):
        self.reader.messages.append(Message(field=field, value=value, message=message, message_raw=message_raw, filename=self.filename, line_no=self.line_no))

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

# ======================================================================

def parse_name(name: str, metadata: dict, context: Context):
    preprocessed_name = context.preprocess_virus_name(name, metadata)
    if preprocessed_name[:10] == "<no-parse>":
        return preprocessed_name[10:]
    else:
        # print(f">>>> {preprocessed_name}", file=sys.stderr)
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
                context.message(field="name", value=value, message=f"[{message.type}] {message.value} -- {message.context}", message_raw=message)
            context.unrecognized_locations(result.messages.unrecognized_locations())
            return name

# ----------------------------------------------------------------------

def parse_date(date: str, metadata: dict, context: Context):
    preprocessed_date = context.preprocess_date(date, metadata)
    if not preprocessed_date:
        return preprocessed_date
    try:
        return ae_backend.date_format(preprocessed_date, allow_incomplete=True, throw_on_error=True, month_first=metadata.get("lab") == "CDC")
    except Exception as err:
        if date != preprocessed_date:
            value = f"{preprocessed_date} (original: {date})"
        else:
            value = date
        context.message(field="date", value=value, message=str(err))
        print(f">> date not parsed: {value}", file=sys.stderr)
        return date

# ======================================================================
