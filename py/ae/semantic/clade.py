import ae_backend

# ======================================================================

def clades(chart: ae_backend.chart_v3.Chart, style_name: str, entries: list, modifiers: dict = {"outline": "black", "rais": True, "only": "antigens"}, add_counter: bool = True) -> str:
    extra_clades = {}
    sname = f"-{style_name}"
    style = chart.styles()[sname]
    style.priority = 1000
    legend_priority = 99
    style_modifier_added = False
    for modifier in entries:
        if clade := modifier.get("clade"):
            if aa := modifier.get("aa"):
                extra_clade = f"{clade} {aa}"
                extra_clades[extra_clade] = {"base_clade": clade, "aa": aa}
                clade = extra_clade
            style.add_modifier(selector=["C", clade], fill=modifier["fill"], **modifiers, legend=modifier["legend"], legend_priority=legend_priority)
        else:
            print(f">> clades: unsupported style \"{style_name}\": {entries}", file=sys.stderr)
        legend_priority -= 1
    style.legend.add_counter = add_counter

    for clade_name, data in extra_clades.items():
        for selector in [chart.select_antigens, chart.select_sera]:
            for no, ag_sr in selector(lambda en: en.has_clade(data["base_clade"]) and en.aa[data["aa"]]):
                ag_sr.semantic.add_clade(clade_name)

    return sname

# ======================================================================
