from pathlib import Path
from typing import Optional
import ae_backend.chart_v3

# ======================================================================

def merge(sources: list[Path]|list[ae_backend.chart_v3.Chart], match: str, merge_type: str, combine_cheating_assays: bool, duplicates_distinct: bool, report: bool) -> ae_backend.chart_v3.Chart:
    """match: "strict", "relaxed", "ignored", "auto"
    merge_type: "type1", "simple", "type2", "incremental", "type3", "overlay", "type4", "type5"
    """

    def get(src: Path|ae_backend.chart_v3.Chart) -> ae_backend.chart_v3.Chart:
        if not isinstance(src, ae_backend.chart_v3.Chart):
            src = ae_backend.chart_v3.Chart(src)
        if duplicates_distinct:
            src.duplicates_distinct()
        return src

    def report_chart(chart: ae_backend.chart_v3.Chart) -> str:
        return f"{chart.name()} {chart.number_of_antigens()}:{chart.number_of_sera()}"

    if len(sources) < 2:
        raise ValueError("too few sources")
    if match not in ["strict", "relaxed", "ignored", "auto"]:
        raise ValueError("invalid match")
    if merge_type not in ["type1", "simple", "type2", "incremental", "type3", "overlay", "type4", "type5"]:
        raise ValueError("invalid merge_type")

    merge: ae_backend.chart_v3.Chart = get(sources[0])
    for src in sources[1:]:
        chart = get(src)
        merge, merge_data = ae_backend.chart_v3.merge(merge, chart, match=match, merge_type=merge_type, combine_cheating_assays=combine_cheating_assays)
        if report:
            print(report_chart(merge), report_chart(chart), merge_data.common(), "-" * 70, sep="\n", end="\n\n")
    return merge

# ======================================================================
