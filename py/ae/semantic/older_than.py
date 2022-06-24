import sys, datetime, calendar
import ae_backend

# ======================================================================

def attributes(chart: ae_backend.chart_v3.Chart, conferencence_date: datetime.date):
    """add semantic attributes "o6m" and "o12m" to test antigens that are older than 6 months and 12 months at the first day of month of the conference"""
    ags = {"o6m": 0, "o12m": 0, "!o6m": 0, "!o12m": 0}
    for attr, last_date in [["o6m", older_6_months_date(conferencence_date).strftime("%Y-%m-%d")], ["o12m", older_12_months_date(conferencence_date).strftime("%Y-%m-%d")]]:
        ags[f"{attr}-last"] = last_date
        for no, ag in chart.select_all_antigens():
            if ag.date() < last_date:
                ag.semantic.set(attr, True)
                ags[attr] += 1
            else:
                ags[f"!{attr}"] += 1
    print(f">>>> {ags}", file=sys.stderr)

def older_6_months_date(conferencence_date: datetime.date) -> datetime.date:
    date = conferencence_date.replace(day=1)
    if date.month > 6:
        return date.replace(month=date.month - 6)
    else:
        return date.replace(year=date.year - 1, month=date.month + 6)

def older_12_months_date(conferencence_date: datetime.date) -> datetime.date:
    return conferencence_date.replace(year=conferencence_date.year - 1, day=1)

# ======================================================================

def style(chart: ae_backend.chart_v3.Chart):
    """Add "-o6m-grey" and "-o12m-grey" styles"""
    for attr in ["o6m", "o12m"]:
        sname = f"-{attr}-grey"
        style = chart.styles()[sname]
        style.priority = 1000
        style.add_modifier(selector={attr: True}, outline="grey", fill="grey", lower=True, only="antigens")

def since_6m_label(conferencence_date: datetime.date) -> str:
    date = older_6_months_date(conferencence_date=conferencence_date)
    return f"{calendar.month_name[date.month]} {date.year}"

def since_12m_label(conferencence_date: datetime.date) -> str:
    date = older_12_months_date(conferencence_date=conferencence_date)
    return f"{calendar.month_name[date.month]} {date.year}"

# ======================================================================
