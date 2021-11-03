from pathlib import Path

from .messages import Message

import ae.utils.directory_module

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
