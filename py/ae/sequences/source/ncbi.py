import sys, re, pprint
from pathlib import Path

import ae_backend
from .context import Context

# ======================================================================

class Error (RuntimeError): pass

# ----------------------------------------------------------------------

class reader:

    def __init__(self, filename: Path):
        self.reader_ = ae_backend.FastaReader(filename)
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
                gisaid_name_parser(en.name, context=context) \
                or naomi_name_parser(en.name, context=context) \
                or regular_name_parser(en.name, lab_hint=self.lab_hint, context=context)
            yield metadata, en.sequence # metadata may contain "excluded" key to manually exclude the sequence

# ----------------------------------------------------------------------
