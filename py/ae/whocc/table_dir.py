import os
from pathlib import Path
import ae_backend

# ======================================================================

WHOCC_TABLES_DIR = Path(os.environ.get("WHOCC_TABLES_DIR"))
if not WHOCC_TABLES_DIR:
    raise RuntimeError(f"""WHOCC_TABLES_DIR env var not set""")

# ======================================================================

def subtype_assay_lab_output_dir(extractor: ae_backend.whocc.xlsx.Extractor):
    return WHOCC_TABLES_DIR.joinpath(extractor.format_assay_data("{virus_type_lineage}-{assay_low_rbc}-{lab_low}"))

def subtype_assay_lab_stem(extractor: ae_backend.whocc.xlsx.Extractor):
    return extractor.format_assay_data("{virus_type_lineage}-{assay_low_rbc}-{lab_low}-{table_date:%Y%m%d}")

# ----------------------------------------------------------------------

def subtype_assay_lab_torg_pathname(extractor: ae_backend.whocc.xlsx.Extractor, torg_dir: Path = None):
    if not torg_dir:
        torg_dir = subtype_assay_lab_output_dir(extractor=extractor).joinpath("torg")
    if not torg_dir.exists():
        raise RuntimeError(f"""Torg output dir "{torg_dir}" does not exist""")
    return torg_dir.joinpath(subtype_assay_lab_stem(extractor=extractor) + ".torg")

def subtype_assay_lab_xlsx_pathname(extractor: ae_backend.whocc.xlsx.Extractor, xlsx_dir: Path = None):
    if not xlsx_dir:
        xlsx_dir = subtype_assay_lab_output_dir(extractor=extractor).joinpath("xlsx")
    if not xlsx_dir.exists():
        raise RuntimeError(f"""Xlsx output dir "{xlsx_dir}" does not exist""")
    return xlsx_dir.joinpath(subtype_assay_lab_stem(extractor=extractor) + ".xlsx")

# ----------------------------------------------------------------------

def subtype_assay_lab_ace_pathname(extractor: ae_backend.whocc.xlsx.Extractor, prn_read: bool = False, ace_dir: Path = None):
    if not ace_dir:
        output_dir = subtype_assay_lab_output_dir(extractor=extractor)
    else:
        output_dir = ace_dir
    if not output_dir.exists():
        raise RuntimeError(f"""ace output dir "{output_dir}" does not exist""")
    if prn_read:
        if ace_dir:
            prn_read_dir = ace_dir
        else:
            prn_read_dir = ace_dir.joinpath("prn-read")
            if not prn_read_dir.exists():
                raise RuntimeError(f"""prn_read_dir "{prn_read_dir}" does not exist""")
    ace_filename = subtype_assay_lab_stem(extractor=extractor) + ".ace"
    return [
        output_dir.joinpath(ace_filename),
        prn_read_dir.joinpath(ace_filename) if prn_read else None
    ]

# ----------------------------------------------------------------------

def subtype_assay_lab_data_fix_pathname(extractor: ae_backend.whocc.xlsx.Extractor):
    return subtype_assay_lab_output_dir(extractor).joinpath("ae-whocc-data-fix.py")

# ----------------------------------------------------------------------

def detect_pathname():
    return WHOCC_TABLES_DIR.joinpath("ae-whocc-detect.py")

# ======================================================================
