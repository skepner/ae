import ae_backend

# ======================================================================

class DataFix:
    "Base class for lab/subtype specific datafixer in modules located in ${WHOCC_TABLES_DIR}/subype-assy-lab"

    def __init__(self, extractor: ae_backend.whocc.xlsx.Extractor):
        self.extractor = extractor

    def lab(self, lab):
        return lab

    def assay(self, assay):
        return assay

    def subtype(self, subtype):
        return subtype

    def rbc(self, rbc):
        return rbc

    def lineage(self, lineage):
        return lineage

    # ----------------------------------------------------------------------

    def antigen(self, antigen: dict, antigen_no: int):
        return antigen

    def serum(self, serum: dict, serum_no: int):
        return serum

    def titer(self, titer: str, antigen_no: int, serum_no: int):
        return titer

# ======================================================================
