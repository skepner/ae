import io, pprint
from pathlib import Path
import ae_backend
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
        data[sr_row.index("name")][sr_col] = " ".join([serum["name"], serum["conc"], serum["dilut"], "BOOSTED" if serum["boosted"] else ""]).strip()
        data[sr_row.index("passage")][sr_col] = serum["passage"]
        data[sr_row.index("serum_id")][sr_col] = serum["serum_id"]

    multivalue_titer = False
    for ag_no in range(extractor.number_of_antigens()):
        ag_row = sr_row.index("base") + ag_no
        antigen = data_fixer.antigen(extractor.antigen(ag_no), ag_no)
        data[ag_row][ag_col.index("name")] = antigen["name"]
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
