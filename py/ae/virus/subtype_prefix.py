import ae_backend

# ======================================================================

def add_subtype_prefix(chart: ae_backend.chart_v3.Chart, name: str) -> str:
    if name and name[:2] not in ["A/", "A(", "B/"]:
        name = f"{chart.info().type_subtype()}/{name}"
    return name

# ======================================================================
