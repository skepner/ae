import sys, io, re, math, pprint, datetime
from pathlib import Path
import ae_backend.whocc
from ..utils import open_file, load_module
from . import data_fix, table_dir

# ======================================================================

def export_to_file(extractor: ae_backend.whocc.xlsx.Extractor, output_dir: Path = None):
    if (data_fix_module_filename := table_dir.subtype_assay_lab_data_fix_pathname(extractor=extractor)).exists():
        data_fixer = load_module.load(data_fix_module_filename).DataFix(extractor)
    else:
        print(f">> \"{data_fix_module_filename}\" does not exist")
        data_fixer = data_fix.DataFix(extractor)
    filename = table_dir.subtype_assay_lab_torg_pathname(extractor=extractor, torg_dir=output_dir)
    print(f">>> {filename}")
    with open_file.for_writing(filename) as output:
        info = generate(extractor=extractor, data_fixer=data_fixer, output=output)
    info["torg_filename"] = filename
    return info

# ----------------------------------------------------------------------

def generate(extractor: ae_backend.whocc.xlsx.Extractor, data_fixer: data_fix.DataFix, output: io.FileIO):
    header_data, assay = header(extractor=extractor, data_fixer=data_fixer)
    print("# -*- Org -*-\n",
          "\n".join(f"- {k}: {v}" for k, v in header_data),
          "",
          sep="\n", file=output)

    data, multivalue_titer = table(extractor=extractor, data_fixer=data_fixer)
    column_widths = [max(len(row[col_no]) for row in data) for col_no in range(len(data[0]))]
    for row in data:
        print("| ",
              " | ".join(f"{row[col_no]:{column_widths[col_no]}s}" for col_no in range(len(row))),
              " |",
              sep="", file=output)

    print("",
          "* COMMENT local vars ----------------------------------------------------------------------",
          ":PROPERTIES:",
          ":VISIBILITY: folded",
          ":END:",
          "#+STARTUP: showall indent",
          "Local Variables:",
          """eval: (if (fboundp 'eu-whocc-torg-to-ace) (add-hook 'after-save-hook 'eu-whocc-torg-to-ace nil 'local))""",
          """eval: (if (fboundp 'eu-whocc-xlsx-torg-ace-hup) (add-hook 'after-save-hook 'eu-whocc-xlsx-torg-ace-hup nil 'local))""",
          "End:",
          sep="\n", file=output)

    return {"assay": assay, "multivalue_titer": multivalue_titer}

# ----------------------------------------------------------------------

def header(extractor: ae_backend.whocc.xlsx.Extractor, data_fixer: data_fix.DataFix):
    assay = data_fixer.assay(extractor.assay())
    header = [
        ["Lab", data_fixer.lab(extractor.lab())],
        ["Date", extractor.format_assay_data("{table_date:%Y-%m-%d}")],
        ["Assay", assay],
        ["Subtype", data_fixer.subtype(extractor.subtype_without_lineage())],
        ]
    if rbc := data_fixer.rbc(extractor.rbc()):
        header.append(["Rbc", rbc])
    if lineage := data_fixer.lineage(extractor.lineage()):
        header.append(["Lineage", lineage])
    return header, assay

# ----------------------------------------------------------------------

def table(extractor: ae_backend.whocc.xlsx.Extractor, data_fixer: data_fix.DataFix):
    ag_col = ["serum_field_name", "name", "date", "passage", "lab_id", "base"]
    sr_row = ["antigen_field_name", "name", "passage", "serum_id", "base"]

    data = []
    for row_no in range(extractor.number_of_antigens() + sr_row.index("base")):
        data.append(["" for col_no in range(extractor.number_of_sera() + ag_col.index("base"))])
    data[0][ag_col.index("name")] = "name"
    data[0][ag_col.index("date")] = "date"
    data[0][ag_col.index("passage")] = "passage"
    data[0][ag_col.index("lab_id")] = "lab_id"
    data[   sr_row.index("name")][0] = "name"
    data[   sr_row.index("passage")][0] = "passage"
    data[   sr_row.index("serum_id")][0] = "serum_id"

    for sr_no in range(extractor.number_of_sera()):
        sr_col = ag_col.index("base") + sr_no
        serum = data_fixer.serum(extractor.serum(sr_no), sr_no)
        if not serum["name"] or not  serum["serum_id"]:
                print(f">> SR {sr_no} {serum} {extractor.serum(sr_no)}", file=sys.stderr)
        data[sr_row.index("name")][sr_col] = " ".join([serum["name"], serum["conc"], serum["dilut"], "BOOSTED" if serum["boosted"] else ""]).strip()
        data[sr_row.index("passage")][sr_col] = serum["passage"]
        data[sr_row.index("serum_id")][sr_col] = serum["serum_id"]

    multivalue_titer = False
    for ag_no in range(extractor.number_of_antigens()):
        ag_row = sr_row.index("base") + ag_no
        antigen = data_fixer.antigen(extractor.antigen(ag_no), ag_no)
        data[ag_row][ag_col.index("name")] = " ".join([antigen["name"], antigen["annotations"]])
        data[ag_row][ag_col.index("date")] = antigen["date"]
        data[ag_row][ag_col.index("passage")] = antigen["passage"]
        data[ag_row][ag_col.index("lab_id")] = antigen["lab_id"]

        for sr_no in range(extractor.number_of_sera()):
            titer = data_fixer.titer(extractor.titer(ag_no, sr_no), ag_no, sr_no)
            fields = titer.split("/")
            if len(fields) == 2:
                data[ag_row][ag_col.index("base") + sr_no] = f"{fields[0]:>5s} / {fields[1]:>5s}"
                multivalue_titer = True
            else:
                data[ag_row][ag_col.index("base") + sr_no] = f"{titer:>5s}"

    # pprint.pprint(data, width=200)
    return data, multivalue_titer

# ======================================================================

def to_ace(torg_filename: Path, prn_read: bool, prn_remove_concentration: bool = False) -> dict:
    lines = [line.decode("utf-8").strip() for line in open_file.for_reading(torg_filename)]
    lab = None
    subtype = None
    assay = None
    # num_antigens = 0
    # num_sera = 0
    ace = {
        "  version": "acmacs-ace-v1",
        "?created": f"imported from {torg_filename.name} by {Path(sys.argv[0]).name} on {datetime.date.today()}",
        "c": {
            "i": {}, "a": [], "s": [], "t": {"l": []}
            }
    }
    for line in lines:
        if line and line[0] == '-':
            field, value = line[1:].strip().split(":")
            field = field.strip().lower()
            value = value.strip()
            if field == "lab":
                ace["c"]["i"]["l"] = value
                lab = value.upper()
            elif field == "date":
                ace["c"]["i"]["D"] = value.replace("-", "")
            elif field == "subtype":
                ace["c"]["i"]["V"] = value
                subtype = value.upper()
            elif field == "assay":
                ace["c"]["i"]["A"] = value
                assay = value.upper()
            elif field == "rbc":
                ace["c"]["i"]["r"] = value
            elif field == "lineage":
                ace["c"]["i"]["s"] = value
            elif field == "error":
                raise RuntimeError(f"> ERROR: {value}")
            else:
                raise RuntimeError(f"Unrecognized meta field name: \"{field}\"")

    table_rows = [no for no, line in enumerate(lines, start=1) if line and line[0] == "|"]
    rows = [[cell.strip() for cell in line[1:-1].split("|")] for line in lines if line and line[0] == "|"]
    if not rows:
        raise RuntimeError(f"no table present, table_rows: {table_rows}")
    # pprint.pprint(rows)
    ace_antigen_fields = {}
    serum_columns = []
    for no, cell in enumerate(rows[0][1:], start=1):
        if not cell:
            serum_columns.append(no)
        else:
            k, v = antigen_field(no, cell.lower())
            if k:
                ace_antigen_fields[k] = v
    ace_serum_fields = {}
    antigen_rows = []
    for no, row in enumerate(rows[1:], start=1):
        cell = row[0]
        if not cell:
            antigen_rows.append(no)
        else:
            k, v = serum_field(no, cell.lower())
            if k:
                ace_serum_fields[k] = v
    # print(f"antigens: {ace_antigen_fields} {len(antigen_rows)} {antigen_rows}")
    # print(f"sera: {ace_serum_fields} {len(serum_columns)} {serum_columns}")
    # pprint.pprint(ace_antigen_fields)

    # Lineage (sera)
    for column_no in serum_columns:
        entry = {}
        for field, row_no in ace_serum_fields.items():
            value = make_antigen_serum_field(field, rows[row_no][column_no], lab)
            if value is not None:
                entry[field] = value
        if ace["c"]["i"].get("V") == "B" and ace["c"]["i"].get("s"): # lineage
            entry["L"] = ace["c"]["i"]["s"][0]
        if prn_remove_concentration:
            remove_concentration(entry)
        ace["c"]["s"].append(entry)

    # Lineage (antigens) and Titers
    for row_no in antigen_rows:
        entry = {}
        for field, column_no in ace_antigen_fields.items():
            value = make_antigen_serum_field(field, rows[row_no][column_no], lab)
            if value is not None:
                entry[field] = value
        if ace["c"]["i"].get("V") == "B" and ace["c"]["i"].get("s"): # lineage
            entry["L"] = ace["c"]["i"]["s"][0]
        if prn_remove_concentration:
            remove_concentration(entry)
        ace["c"]["a"].append(entry)
        ace["c"]["t"]["l"].append([check_titer(rows[row_no][column_no], torg_filename, table_rows[row_no], column_no, prn_read=prn_read, lab=lab, subtype=subtype, assay=assay, warn_if_not_normal=True) for column_no in serum_columns])

    detect_reference(ace["c"]["a"], ace["c"]["s"])

    return ace

# ----------------------------------------------------------------------

def antigen_field(no, cell):
    if cell == "name":
        return "N", no
    elif cell == "date":
        return "D", no
    elif cell == "passage":
        return "P", no
    elif cell == "reassortant":
        return "R", no
    elif cell == "lab_id":
        return "l", no
    elif cell == "annotation":
        return "a", no
    elif cell == "clade":
        return "c", no
    elif cell[0] == "#":
        return None, no
    else:
        raise RuntimeError(f"Unrecognized antigen (first row, column {no}) field name: \"{cell}\"")

# ----------------------------------------------------------------------

sReDelim = re.compile(r"^[\-\+]+$")

def serum_field(no, cell):
    if cell == "name":
        return "N", no
    elif cell == "passage":
        return "P", no
    elif cell == "reassortant":
        return "R", no
    elif cell == "serum_id":
        return "I", no
    elif cell == "annotation":
        return "a", no
    elif cell == "species":
        return "s", no
    elif cell[0] == "#" or sReDelim.match(cell):
        return None, no
    else:
        raise RuntimeError(f"Unrecognized serum (first column, row {no}) field name: \"{cell}\"")

# ----------------------------------------------------------------------

def make_antigen_serum_field(key, value, lab):
    if key in ["l", "a"]:
        if value:
            if key == "l" and not value.startswith(lab):
                value = f"{lab}#{value}"
            return [value]
        else:
            return None
    elif key == "D":
        if value:
            return value
        else:
            return None
    else:
        return value

# ----------------------------------------------------------------------

sReTiter = re.compile(r"^([><]?\d+|\*)$", re.I)
sNormalTiters = ["10", "20", "40", "80", "160", "320", "640", "1280", "2560", "5120", "10240",
                 "<10", "<20", "<40", "<80",
                 ">1280", ">2560", ">5120", ">10240",
                 "*"
                 ]

def check_titer(titer, filename, row_no, column_no, lab, subtype, assay, prn_read=False, warn_if_not_normal=True, convert_prn_low_read=False, prn_titer=None):
    if sReTiter.match(titer):
        if prn_read:
             raise RuntimeError(f"PRN Read titer is not available: \"{titer}\" @@ {filename}:{row_no} column {column_no + 1}")
        if titer not in sNormalTiters and warn_if_not_normal:
            print(f">> unusual titer \"{titer}\" @@ {filename}:{row_no} column {column_no + 1}", file=sys.stderr)
        if convert_prn_low_read and titer[0] not in ["<", ">", "*"] and int(titer) < 10:
            titer = "<10"
        return titer
    elif titer == "<" and lab == "CRICK" and assay == "PRN":
        if convert_prn_low_read:
            return "<10"
        elif prn_titer:
            if prn_titer == "<" or int(prn_titer) < 10:
                return "<10"
            else:
                prn_log = math.log2(int(prn_titer) / 10.0)
                if math.ceil(prn_log) == prn_log:
                    prn_log += 1
                else:
                    prn_log = math.ceil(prn_log)
                print(f">>>> {titer} {prn_titer} -> {prn_log} -> {int(math.pow(2, prn_log) * 10)}", file=sys.stderr)
                return "<" + str(int(math.pow(2, prn_log) * 10))
        else:
            return "<40"        # unclear
    elif "/" in titer:
        hi_titer, prn_titer = (tit.strip() for tit in titer.split("/"))
        if prn_read:
            return check_titer(prn_titer, filename, row_no, column_no, lab=lab, subtype=subtype, assay=assay, warn_if_not_normal=False, convert_prn_low_read=True)
        else:
            return check_titer(hi_titer, filename, row_no, column_no, lab=lab, subtype=subtype, assay=assay, prn_titer=prn_titer, warn_if_not_normal=warn_if_not_normal)
    else:
        raise RuntimeError(f"Unrecognized titer \"{titer}\" @@ {filename}:{row_no} column {column_no + 1}")

# ----------------------------------------------------------------------

# Crick PRN tables have concentration in passage or name, e.g. (10-4), it is perhaps property of the assay and not the property (annotation) of an antigen/serum
# see Derek's message with subject "Crick H3 VN tables" date 2018-02-14 15:20
sReConc = re.compile(r"\s+\(?10-\d\)?$")

def remove_concentration(entry):
    entry["N"] = sReConc.sub("", entry["N"])
    entry["P"] = sReConc.sub("", entry["P"])

# ----------------------------------------------------------------------

def fix_name_for_comparison(name):
    fields = name.upper().split("/")
    try:
        year = int(fields[-1])
    except ValueError:
        year = None
    if year:
        if year < 50:
            year += 2000
        elif year < 100:
            year += 1900
        fields[-1] = str(year)
    return "/".join(fields)

def detect_reference(antigens, sera):
    # print("\n".join(fix_name_for_comparison(antigen["N"]) for antigen in antigens), "\n\n", "\n".join(fix_name_for_comparison(serum["N"]) for serum in sera), sep="")
    for antigen in antigens:
        if any(fix_name_for_comparison(serum["N"]) == fix_name_for_comparison(antigen["N"]) for serum in sera):
            antigen["S"] = "R"

# ======================================================================
