import sys
import ae_backend

# ======================================================================

def semantic(chart: ae_backend.chart_v3.Chart):
    """Set continent ("C9") and country ("c9") semantic attributes for all antigens and sera"""
    locdb = ae_backend.locdb_v3.locdb()

    loc = "CAMBRIDGE"
    print(f"{loc} -- {locdb.country(loc)} -- {locdb.continent(locdb.country(loc))}", file=sys.stderr)

# ======================================================================
