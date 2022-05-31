import sys, datetime, calendar
import ae_backend

# ======================================================================

def parse_date(source: datetime.date|str, default_month: int = 1, default_day: int = 1) -> datetime.date:
    """parses date from strings like: YYYY-MM-DD, YYYYMMDD, YYYY, YYYY-MM, YYYYMM"""
    if isinstance(source, datetime.date):
        return source
    elif len(source) == 4:        # YYYY
        return datetime.date.fromisoformat(f"{source}-{default_month:02d}-{default_day:02d}")
    elif len(source) == 6:        # YYYYMM
        return datetime.date.fromisoformat(f"{source[:4]}-{source[4:]}-{default_day:02d}")
    elif len(source) == 7:        # YYYY-MM
        return datetime.date.fromisoformat(f"{source}-{default_day:02d}")
    elif len(source) == 8:        # YYYYMMDD
        return datetime.date.fromisoformat(f"{source[:4]}-{source[4:6]}-{source[6:]}")
    elif len(source) == 10:        # YYYY-MM-DD
        return datetime.date.fromisoformat(source)
    else:
        raise ValueError(f"cannot parse date from \"{source}\"")

# ----------------------------------------------------------------------

def get_antigen_date_range(chart: ae_backend.chart_v3.Chart, first: datetime.date|str, last: datetime.date|str) -> [datetime.date, datetime.date]:
    chart_first, chart_last = (parse_date(date) for date in chart.antigen_date_range(test_only=True))
    if not first:
        first = chart_first
    else:
        first = parse_date(first)
        if first < chart_first:
            first = chart_first
    if not last:
        last = chart_last
    else:
        last = parse_date(last)
        if last > chart_last:
            last = chart_last
    # print(f">>>> antigen_date_range {repr(first)} {repr(last)} -- chart: {chart_first} {chart_last}", file=sys.stderr)
    return [first, last]

# ======================================================================

def time_series(first: datetime.date, last_inclusive: datetime.date, period: str) -> [[datetime.date, datetime.date]]:
    """period: "year", "month", "week". return list of pairs [first, last] for each interval in time series."""
    if first >= last_inclusive:
        raise ValueError(f"time_series: invalid date range: {first} {last_inclusive}\"")
    if period == "year":
        return [[datetime.date(year, 1, 1), datetime.date(year + 1, 1, 1)] for year in range(first.year, last_inclusive.year + 1)]
    elif period == "month":
        last = last_inclusive + datetime.timedelta(days=calendar.monthrange(last_inclusive.year, last_inclusive.month)[1])
        if first.year == last.year:
            return [[datetime.date(first.year, month, 1), datetime.date(first.year, month + 1, 1)] for month in range(first.month, last.month)]
        else:
            ts = [[datetime.date(first.year, month, 1), datetime.date(first.year, month + 1, 1)] for month in range(first.month, 12)] + [[datetime.date(first.year, 12, 1), datetime.date(first.year + 1, 1, 1)]]
            for year in range(first.year + 1, last.year):
                ts += [[datetime.date(year, month, 1), datetime.date(year, month + 1, 1)] for month in range(1, 12)] + [[datetime.date(year, 12, 1), datetime.date(year + 1, 1, 1)]]
            if last.month > 1:
                ts += [[datetime.date(last.year, month, 1), datetime.date(last.year, month + 1, 1)] for month in range(1, last.month)]
            return ts
    elif period == "week":
        first = first - datetime.timedelta(days=calendar.weekday(year=first.year, month=first.month, day=first.day))
        last = last_inclusive + datetime.timedelta(days=calendar.weekday(year=last_inclusive.year, month=last_inclusive.month, day=last_inclusive.day) + 7)
        ts = []
        while first < last:
            next = first + datetime.timedelta(days=7)
            ts += [[first, next]]
            first = next
        return ts
    else:
        raise ValueError(f"time_series: uknown period: \"{period}\"")

# ======================================================================
