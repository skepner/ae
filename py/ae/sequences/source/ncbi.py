import sys, re, pprint
from pathlib import Path

import ae_backend
from .context import Context

# ======================================================================

class Error (RuntimeError): pass

# ----------------------------------------------------------------------

class reader:

    def __init__(self, ncbi_dir: Path):
        na_dat = self.read_influenza_na_dat(ncbi_dir.joinpath("influenza_na.dat.xz"))

    def __iter__(self):
        for en in self.reader_:
            context = Context(self, filename=Path(en.filename), line_no=en.line_no)
            metadata = \
                gisaid_name_parser(en.name, context=context) \
                or naomi_name_parser(en.name, context=context) \
                or regular_name_parser(en.name, lab_hint=self.lab_hint, context=context)
            yield metadata, en.sequence # metadata may contain "excluded" key to manually exclude the sequence

    # ----------------------------------------------------------------------

    def read_influenza_na_dat(self, filename: Path):
        data = ae_backend.read_file(filename)
        # print(f"{filename}: {len(data)}")

# ----------------------------------------------------------------------
