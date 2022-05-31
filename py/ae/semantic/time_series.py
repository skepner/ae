import sys, datetime, pprint
import ae_backend
from ..utils.datetime import get_antigen_date_range, time_series

# ======================================================================

def style(chart: ae_backend.chart_v3.Chart, name: str = "-ts-", first: datetime.date|str = None, last: datetime.date|str = None, period: str = "month", first_priority: int = 5000) -> set[str]:
    """period: "year", "month", "week" """

    if period == "year":
        name_format = "%Y"
    elif period == "month":
        name_format = "%Y-%m"
    elif period == "week":
        name_format = "%Y-%m-%d"
    else:
        raise ValueError(f"time_series: uknown period: \"{period}\"")

    snames = set()
    first, last = get_antigen_date_range(chart=chart, first=first, last=last)
    priority = first_priority
    for dfirst, dlast in time_series(first=first, last_inclusive=last, period=period):
        sname = f"{name}{dfirst.strftime(name_format)}"
        style = chart.styles()[sname]
        style.priority = priority
        style.add_modifier(selector={"R": False, "!D": ["", dfirst.strftime("%Y-%m-%d")]}, hide=True, only="antigens")
        style.add_modifier(selector={"R": False, "!D": [dlast.strftime("%Y-%m-%d"), ""]}, hide=True, only="antigens")
        snames.add(sname)
        priority += 1

    # pprint.pprint(time_series(first=datetime.date(2022, 6, 1), last_inclusive=datetime.date(2023, 1, 2), period="week"))
    # pprint.pprint(time_series(first=first, last_inclusive=last, period="month"))
    return snames

# ======================================================================
