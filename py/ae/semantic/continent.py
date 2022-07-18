import sys
import ae_backend
from ae.utils.org import org_table_to_dict

# ======================================================================

def attributes(chart: ae_backend.chart_v3.Chart):
    """Set continent ("C9") and country ("c9") semantic attributes for all antigens and sera"""
    for selector in [chart.select_all_antigens, chart.select_all_sera]:
        for ag_no, antigen in selector():
            if parsed := ae_backend.virus.name_parse(antigen.name()):
                if parsed.parts.continent:
                    antigen.semantic.set("C9", parsed.parts.continent)
                if parsed.parts.country:
                    antigen.semantic.set("c9", parsed.parts.country)

# ======================================================================

sContinentStyle = {
    "normal": org_table_to_dict("""
# -*- Org -*-
| continent         | color   |
|-------------------+---------|
| EUROPE            | #00FF00 |
| CENTRAL-AMERICA   | #AAF9FF |
| MIDDLE-EAST       | #8000FF |
| NORTH-AMERICA     | #00008B |
| AFRICA            | #FF8000 |
| ASIA              | #FF0000 |
| RUSSIA            | #B03060 |
| AUSTRALIA-OCEANIA | #FF69B4 |
| SOUTH-AMERICA     | #40E0D0 |
| ANTARCTICA        | #808080 |
| CHINA-SOUTH       | #FF0000 |
| CHINA-NORTH       | #6495ED |
| CHINA-UNKNOWN     | #808080 |
| UNKNOWN           | #808080 |
# -*-
        """),

    "dark": org_table_to_dict("""
# -*- Org -*-
| continent         | color   |
|-------------------+---------|
| EUROPE            | #00A800 |
| CENTRAL-AMERICA   | #70A4A8 |
| MIDDLE-EAST       | #8000FF |
| NORTH-AMERICA     | #00008B |
| AFRICA            | #FF8000 |
| ASIA              | #FF0000 |
| RUSSIA            | #B03060 |
| AUSTRALIA-OCEANIA | #FF69B4 |
| SOUTH-AMERICA     | #40E0D0 |
| ANTARCTICA        | #808080 |
| CHINA-SOUTH       | #FF0000 |
| CHINA-NORTH       | #6495ED |
| CHINA-UNKNOWN     | #808080 |
| UNKNOWN           | #808080 |
# -*-
        """)
    }

def style(chart: ae_backend.chart_v3.Chart, name: str = "-continent", test_only: bool = True, raise_: bool = False, style_type: str = "normal", priority: int = 1000) -> set[str]:
    style = chart.styles()[name]
    style.priority = priority
    if test_only:
        reference_selector = {"R": False}
    else:
        reference_selector = {}
    for modifier in sContinentStyle[style_type]:
        style.add_modifier(selector={"C9": modifier["continent"], **reference_selector}, fill=modifier["color"], outline="black", raise_=raise_, only="antigens")
    return set([name])

# ======================================================================
