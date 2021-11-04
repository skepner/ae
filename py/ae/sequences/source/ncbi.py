import sys, re, io, pprint
from pathlib import Path

import ae_backend
from .parse import Context, parse_name, parse_date
from ...utils.timeit import timeit

# ======================================================================

class Error (RuntimeError): pass

# ----------------------------------------------------------------------

class reader:

    def __init__(self, ncbi_dir: Path):
        self.messages = []
        self.unrecognized_locations = set()
        self.na_dat_filename = ncbi_dir.joinpath("influenza_na.dat.xz")
        self.fna_filename = ncbi_dir.joinpath("influenza.fna.xz")

    def __iter__(self):
        self.reader_ = ae_backend.FastaReader(self.fna_filename)
        self.na_dat = self.read_influenza_na_dat(ncbi_dir.joinpath("influenza_na.dat.xz"))
        for en in self.reader_:
            context = Context(self, filename=self.fna_filename, line_no=en.line_no)
            metadata = self.read_fna_name(en.name, context=context)
            yield metadata, en.sequence # metadata may contain "excluded" key to manually exclude the sequence

    # ----------------------------------------------------------------------

    def read_fna_name(self, name: str, context: Context):
        return {"name": name}

    # ----------------------------------------------------------------------

    def read_influenza_na_dat(self, filename: Path):
        raw_data = ae_backend.read_file(filename)
        with timeit(f"{filename}"):
            metadata = [entry for entry in (self.read_influenza_na_dat_entry(filename=filename, line_no=line_no, line=line) for line_no, line in enumerate(io.StringIO(raw_data), start=1)) if entry]
            print(f">>> {len(metadata)} rows read from {filename}")
        return metadata

        # print(f"{filename}: {len(data)}")

    def read_influenza_na_dat_entry(self, filename: Path, line_no: int, line: str):
        context = Context(self, filename=filename, line_no=line_no)
        fields = line.split("\t")
        if len(fields) != 11:
            print(f">> invalid number of fields ({len(fields)}) @@ {filename}:{line_no}")
            return None
        if len(fields) == 11 and fields[2] == "4" and (name := self.extract_name(fields[7])): # interested in segment 4 (HA) only
            # genbank_accession, host, segment_no, subtype, country, date, sequence_length, virus_name, age, gender, completeness
            metadata = {
                "sample_id_by_sample_provider": fields[0],
                # "host": fields[1],
                # "gisaid_segment_number": fields[2],
                "subtype": self.parse_subtype(fields[3]),
                "country": self.parse_country(fields[4]),
                # "date": fields[5],
                # "sequence_length": fields[6],
                # "name": name,
                # "age": fields[8],
                # "gender": fields[9],
                # "completeness": fields[10],
                "line_no": line_no,
            }
            metadata["name"] = parse_name(name, metadata=metadata, context=context)
            metadata["date"] = parse_date(fields[5], metadata=metadata, context=context)
            return metadata
        else:
            return None

    def parse_subtype(self, subtype):
        return subtype

    def parse_country(self, country):
        return country

    def extract_name(self, name):
        if name[:19].upper() in ["INFLUENZA A VIRUS (", "INFLUENZA B VIRUS ("] and name[-1] == ")":
            return name[19:-1]
        elif name[:18].upper() in ["INFLUENZA A VIRUS(", "INFLUENZA B VIRUS("] and name[-1] == ")":
            return name[18:-1]
        else:
            # print(f">>>> excluded name \"{name}\"")
            return None

# ----------------------------------------------------------------------
