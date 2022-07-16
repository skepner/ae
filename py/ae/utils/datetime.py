import sys, datetime, calendar
from datetime import datetime, date, timedelta
from typing import Optional
import ae_backend.chart_v3

# ======================================================================

def parse_date(source: date | str, default_month: int = 1, default_day: int = 1) -> date:
    """parses date from strings like: YYYY-MM-DD, YYYYMMDD, YYYY, YYYY-MM, YYYYMM"""
    if isinstance(source, date):
        return source
    elif len(source) == 4:        # YYYY
        return date.fromisoformat(f"{source}-{default_month:02d}-{default_day:02d}")
    elif len(source) == 6:        # YYYYMM
        return date.fromisoformat(f"{source[:4]}-{source[4:]}-{default_day:02d}")
    elif len(source) == 7:        # YYYY-MM
        return date.fromisoformat(f"{source}-{default_day:02d}")
    elif len(source) == 8:        # YYYYMMDD
        return date.fromisoformat(f"{source[:4]}-{source[4:6]}-{source[6:]}")
    elif len(source) == 10:        # YYYY-MM-DD
        return date.fromisoformat(source)
    else:
        raise ValueError(f"cannot parse date from \"{source}\"")

# ----------------------------------------------------------------------

def get_antigen_date_range(chart: ae_backend.chart_v3.Chart, first: Optional[date | str] = None, last: Optional[date | str] = None, limit_by_chart: bool = False) -> list[date]:
    chart_first, chart_last = (parse_date(date) for date in chart.antigen_date_range(test_only=True))
    if not first:
        first = chart_first
    else:
        first = parse_date(first)
        if limit_by_chart and first < chart_first:
            first = chart_first
    if not last:
        last = chart_last
    else:
        last = parse_date(last)
        if limit_by_chart and last > chart_last:
            last = chart_last
    # print(f">>>> antigen_date_range {repr(first)} {repr(last)} -- chart: {chart_first} {chart_last}", file=sys.stderr)
    return [first, last]

# ======================================================================
