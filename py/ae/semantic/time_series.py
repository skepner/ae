import sys, datetime, pprint, calendar
from typing import Optional
import ae_backend
# from ..utils.datetime import get_antigen_date_range
from ..utils.time_series import TimeSeriesRange
from . import front_style
from .style import style_with_one_modifier

# ======================================================================

def style(chart: ae_backend.chart_v3.Chart, time_series: TimeSeriesRange, name: str = "ts-", vaccine_style_name: str = "-vaccines", title_prefix: str = None, title_style: dict[str, object] = {}, priority: int = 5000, front_priority: int = 500):
    """vaccine_style_name - support for different vaccine colring in clades and ts, use -vaccines-ts" for ts"""
    priority_inc = 0
    for dfirst, dlast in time_series.range_begin_end():
        sname = f"-{name}{dfirst.strftime(time_series.name_format_style())}"
        style = chart.styles()[sname]
        style.priority = priority + priority_inc
        style.add_modifier(selector={"R": False, "!D": ["", dfirst.strftime("%Y-%m-%d")]}, hide=True, only="antigens")
        style.add_modifier(selector={"R": False, "!D": [dlast.strftime("%Y-%m-%d"), ""]}, hide=True, only="antigens")
        title = " ".join(en for en in [title_prefix, calendar.month_name[dfirst.month], str(dfirst.year)] if en)
        front_style.add(chart=chart, style_name=sname[1:], references=["-reset", "-continent", "-ts-old-new", sname, vaccine_style_name], title=title, title_style=title_style, show_legend=True, legend_counter=True, style_priority=front_priority + priority_inc)
        priority_inc += 1

# ======================================================================

def style_old_new(chart: ae_backend.chart_v3.Chart, old_size: float, new_size: float, priority: int = 4502):
    """add "-new-1-big" style marking antigens with the "new" semantic attribute"""
    style = chart.styles()["-ts-old-new"]
    style.priority = priority
    style.add_modifier(selector={"R": False}, size=old_size, only="antigens")
    style.add_modifier(selector={"new": 1}, size=new_size, only="antigens", raise_=True)

# ======================================================================
