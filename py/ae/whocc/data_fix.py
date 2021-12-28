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

    # ----------------------------------------------------------------------

    # table: list of lists of two elements: regex, re.sub replacement pattern
    #   e.g. [[re.compile(r"/IRE/(87733/(?:20)?19|84630/(?:20)?18)", re.I), r"/IRELAND/\g<1>"]]
    # ag_sr: "AG" or "SR"
    #   e.g. serum["name"] = self.fix_antigen_serum_field(source=serum["name"], field_name="name", ag_sr="SR", no=serum_no, table=self.sReSerumName)

    def fix_antigen_serum_field(self, source: str, field_name: str, ag_sr: str, no: str, table: list):
        fixed = source
        for rex, replacement in table:
            fixed = rex.sub(replacement, fixed)
        if fixed != source:
            print(f"{ag_sr} {no:3d} {field_name} \"{fixed}\" <- \"{source}\"")
        return fixed

# ======================================================================
