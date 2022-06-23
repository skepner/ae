import sys, calendar
from typing import Optional, Generator

from . import datetime

# ======================================================================

class TimeSeriesRange:

    def __init__(self, first: str|datetime.date, after_last: Optional[str|datetime.date] = None, last_inclusive: Optional[str|datetime.date] = None, period: str = "month"):
        if period not in ["week", "month", "year"]:
            raise ValueError(f"TimeSeriesRange: invalid period: \"{period}\"")
        self.period = period
        self._first = datetime.parse_date(first)
        if self.period == "week":
            self._first -= datetime.timedelta(days=calendar.weekday(year=self._first.year, month=self._first.month, day=self._first.day)) # monday to monday
        if after_last is not None:
            self._after_last = datetime.parse_date(after_last)
        elif last_inclusive is not None:
            self._after_last = self.next(datetime.parse_date(last_inclusive))
        else:
            raise ValueError("either after_last or last_inclusive must be passed")

    def next(self, date: datetime.date) -> datetime.date:
        match self.period:
            case "year":
                return date.replace(year=date.year + 1)
            case "month":
                if date.month == 12:
                    return date.replace(year=date.year + 1, month=1)
                else:
                    return date.replace(month=date.month + 1)
            case "week":
                return date + datetime.timedelta(days=7)
        raise ValueError(f"TimeSeriesRange: invalid period: \"{self.period}\"")

    def previous(self, date: datetime.date) -> datetime.date:
        match self.period:
            case "year":
                return date.replace(year=date.year - 1)
            case "month":
                if date.month == 1:
                    return date.replace(year=date.year - 1, month=12)
                else:
                    return date.replace(month=date.month - 1)
            case "week":
                return date - datetime.timedelta(days=7)
        raise ValueError(f"TimeSeriesRange: invalid period: \"{self.period}\"")

    def range_begin(self) -> Generator[datetime.date, None, None]:
        current = self._first
        while current < self._after_last:
            yield current
            current = self.next(current)

    def range_begin_end(self) -> Generator[list[datetime.date], None, None]:
        current = self._first
        while current < self._after_last:
            nx = self.next(current)
            yield [current, nx]
            current = nx

    def front(self) -> datetime.date:
        return self._first

    def back(self) -> datetime.date:
        return self.previous(self._after_last)

    def after_last(self) -> datetime.date:
        return self._after_last

    def front_YMD(self) -> str:
        return self._first.strftime("%Y-%m-%d")

    def back_YMD(self) -> str:
        return self.previous(self._after_last).strftime("%Y-%m-%d")

    def after_last_YMD(self) -> str:
        return self._after_last.strftime("%Y-%m-%d")

    def __str__(self):
        return f"TimeSeries[{self.front_YMD()}, {self.back_YMD()}]"

    def name_format_style(self) -> str:
        if self.period == "year":
            return "%Y"
        elif self.period == "month":
            return "%Y-%m"
        elif self.period == "week":
            return "%Y-%m-%d"
        else:
            raise ValueError(f"TimeSeriesRange: uknown period: \"{self.period}\"")

# ======================================================================
