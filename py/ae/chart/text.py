import re

import ae_backend.chart_v3, ae_backend.virus
import ae.chart
from ..utils.format_table import format_table, Centered, RightAligned
from ..utils.num_digits import num_digits

# ======================================================================

def text(chart: ae_backend.chart_v3.Chart) -> str:
    # ag_no, name, passage, date, ref, seq, sera...]
    header_prefix = ["", "", "", "", "", ""]
    all_antigens = chart.select_all_antigens()
    ref_ags = [ag_no for ag_no, _ in chart.select_reference_antigens()]
    # ag_no_digits = num_digits(len(all_antigens))
    sequenced_antigens = sum(1 for _, antigen in all_antigens if antigen.sequence_aa())
    all_sera = chart.select_all_sera()
    titers = chart.titers()
    table = [
        header_prefix + [Centered(sr_no) for sr_no in range(chart.number_of_sera())],
        header_prefix + [ae_backend.virus.name_abbreviated_location_isolation_year(sr.name()) for no, sr in all_sera],
        header_prefix + [" ".join(sr.annotations() + [sr.reassortant()]) for no, sr in all_sera],
        header_prefix + [sr.passage() for no, sr in all_sera],
        header_prefix + [sr.serum_id() for no, sr in all_sera],
        ["", "", "", "", "", f"{sequenced_antigens}/{len(all_antigens)}", ""],
    ] + [
        [
            ag_no,
            " ".join([antigen.name(), *antigen.annotations(), antigen.reassortant()]),
            antigen.passage(),
            antigen.date(),
            ":ref" if ag_no in ref_ags else "",
            ":seq" if antigen.sequence_aa() else "",
            *[RightAligned(titers.titer(ag_no, sr_no)) for sr_no in range(chart.number_of_sera())]
        ] for ag_no, antigen in all_antigens]
    return f"{ae.chart.info(chart)}\n{format_table(table, field_sep='  ')}"

# ======================================================================
