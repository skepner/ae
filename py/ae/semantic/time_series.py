import sys, datetime, pprint, calendar
import ae_backend
from ..utils.datetime import get_antigen_date_range, time_series
from . import front_style

# ======================================================================

def style(chart: ae_backend.chart_v3.Chart, name: str = "ts-", first: datetime.date|str = None, last: datetime.date|str = None, title_prefix: str = None, period: str = "month", priority: int = 5000, front_priority: int = 500):
    """period: "year", "month", "week" """

    if period == "year":
        name_format = "%Y"
    elif period == "month":
        name_format = "%Y-%m"
    elif period == "week":
        name_format = "%Y-%m-%d"
    else:
        raise ValueError(f"time_series: uknown period: \"{period}\"")

    first, last = get_antigen_date_range(chart=chart, first=first, last=last)
    priority_inc = 0
    for dfirst, dlast in time_series(first=first, last_inclusive=last, period=period):
        sname = f"-{name}{dfirst.strftime(name_format)}"
        style = chart.styles()[sname]
        style.priority = priority + priority_inc
        style.add_modifier(selector={"R": False, "!D": ["", dfirst.strftime("%Y-%m-%d")]}, hide=True, only="antigens")
        style.add_modifier(selector={"R": False, "!D": [dlast.strftime("%Y-%m-%d"), ""]}, hide=True, only="antigens")
        title = " ".join(en for en in [title_prefix, calendar.month_name[dfirst.month], str(dfirst.year)] if en)
        front_style.add(chart=chart, style_name=sname[1:], references=["-reset", "-continent", "-new-1-big", sname, "-vaccines"], title=title, show_legend=True, legend_counter=True, style_priority=front_priority + priority_inc)
        priority_inc += 1

    # pprint.pprint(time_series(first=datetime.date(2022, 6, 1), last_inclusive=datetime.date(2023, 1, 2), period="week"))
    # pprint.pprint(time_series(first=first, last_inclusive=last, period="month"))

# ======================================================================
