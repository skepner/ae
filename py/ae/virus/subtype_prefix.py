import ae_backend

# ======================================================================

def add_subtype_prefix_for_chart(chart: ae_backend.chart_v3.Chart, name: str) -> str:
    if name and name[:2] not in ["A/", "A(", "B/"]:
        name = f"{chart.info().type_subtype()}/{name}"
    return name
# ----------------------------------------------------------------------

def add_subtype_prefix(type_subtype: str, name: str) -> str:
    if name and name[:2] not in ["A/", "A(", "B/"]:
        name = f"{type_subtype}/{name}"
    return name

# ======================================================================
