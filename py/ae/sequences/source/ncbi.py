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
        self.reader_ = ae_backend.raw_sequence.FastaReader(self.fna_filename)
        self.na_dat = self.read_influenza_na_dat(self.na_dat_filename)
        for en in self.reader_:
            context = Context(self, filename=self.fna_filename, line_no=en.line_no)
            if metadata := self.read_fna_name(en.raw_name, context=context):
                yield metadata, en.sequence # metadata may contain "excluded" key to manually exclude the sequence

    # ----------------------------------------------------------------------

    def read_fna_name(self, name: str, context: Context):
        fields = name.split("|")
        if len(fields) == 5:
            if metadata := self.na_dat.get(fields[3]):
                return metadata
            # if not found, it most probably means wrong segment (not HA)
        else:
            print(f">> [ncbi] invalid fna name: \"{name}\"", file=sys.stderr)
        return None

    # ----------------------------------------------------------------------

    def read_influenza_na_dat(self, filename: Path):
        raw_data = ae_backend.read_file(filename)
        metadata = {entry["sample_id_by_sample_provider"]: entry for entry in (self.read_influenza_na_dat_entry(filename=filename, line_no=line_no, line=line) for line_no, line in enumerate(io.StringIO(raw_data), start=1)) if entry}
        # print(f">>> {len(metadata)} rows read from {filename}", file=sys.stderr)
        return metadata

        # print(f"{filename}: {len(data)}")

    def read_influenza_na_dat_entry(self, filename: Path, line_no: int, line: str):
        context = Context(self, filename=filename, line_no=line_no)
        fields = line.split("\t")
        if len(fields) != 11:
            print(f">> invalid number of fields ({len(fields)}) @@ {filename}:{line_no}", file=sys.stderr)
            return None
        if len(fields) == 11 and fields[2] == "4" and (name := self.extract_name(fields[7])): # interested in segment 4 (HA) only
            # genbank_accession, host, segment_no, subtype, country, date, sequence_length, virus_name, age, gender, completeness
            # print(f">>>> id \"{fields[0]}\"", file=sys.stderr)
            metadata = {
                "sample_id_by_sample_provider": fields[0],
                # "host": fields[1],
                # "gisaid_segment_number": fields[2],
                "type_subtype": self.parse_subtype(fields[3]),
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
            if date := parse_date(fields[5], metadata=metadata, context=context):
                metadata["date"] = date
            # print(f">>>> metadata {metadata}", file=sys.stderr)
            return metadata
        else:
            return None

    sReSubtypeFixMixedH = re.compile(r"^\s*mixed\s*[\.,]\s*H", re.I)

    def parse_subtype(self, subtype):
        subtype = subtype.upper()
        if subtype[:1] == "H":
            subtype = f"A({subtype})"
        elif mm_mixed_h := self.sReSubtypeFixMixedH.match(subtype):
            subtype = f"A(H{mm_mixed_h.end()})"
        elif ",MIXED" in subtype:
            subtype = subtype.replace(",MIXED", "")
        elif subtype == "MIXED" or subtype[:1] == "N":
            subtype = ""
        return subtype

    def parse_country(self, country):
        return country

    def extract_name(self, name):
        try:
            if name[:17].upper() in ["INFLUENZA A VIRUS", "INFLUENZA B VIRUS"] and (paren := name.index("(")) in [17, 18] and name[-1] == ")":
                return name[paren+1:-1]
        except ValueError:
            pass
        return None

# ----------------------------------------------------------------------
