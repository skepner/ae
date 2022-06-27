import re

import ae_backend

# ======================================================================

def info(chart: ae_backend.chart_v3.Chart, show_all: bool) -> str:

    num_projections = chart.number_of_projections()
    nprj = f"projections:{num_projections}" if num_projections else "no projections"

    fcb = None
    if chart_forced_column_bases := chart.forced_column_bases():
        if show_all:
            fcb = "  forced column bases: see after sources below"
        else:
            fcb = f"  forced column bases: {chart_forced_column_bases}"

    projections = ""
    if show_all and num_projections:
        for projection_no in range(num_projections):
            projection = chart.projection(projection_no)
            projections += f"{projection_no:3d}  {projection.stress():11.6f} >={projection.minimum_column_basis()}"
            if comment := projection.comment():
                projections += f" <{comment}>"
            if projection_forced_column_bases := projection.forced_column_bases():
                if projection_forced_column_bases == chart_forced_column_bases:
                    projections += " chart-forced-column-bases"
                else:
                    projections += f" {projection_forced_column_bases}"
            if disconnected := projection.disconnected():
                projections += f" discon:{disconnected}"
            if unmovable := projection.unmovable():
                projections += f" unmov:{unmovable}"
            if unmovable_in_the_last_dimension := projection.unmovable_in_the_last_dimension():
                projections += f" unmov-last:{unmovable_in_the_last_dimension}"
            projections += "\n"

    sources = None
    colbases = None
    if show_all:
        info = chart.info()
        if num_sources := info.number_of_sources():
            sources = f"sources {num_sources}\n"
            for source_no in range(num_sources):
                source = info.source(source_no)
                sources += f"{source_no:3d} {source.date()} {source.name()}\n"
            sources += "\n"
        if chart_forced_column_bases:
            colbases = "forced column bases\n"
            for no, sr in chart.select_all_sera():
                fcb = f"{chart_forced_column_bases[no]:11.8f}"
                fcb = re.sub(r"0+$", lambda m: " " * len(m.group(0)), fcb)
                fcb = re.sub(r"\. ", "  ", fcb)
                colbases += f"{no:3d}     {fcb}   {sr.name()} {sr.serum_id()} {sr.passage()}\n"

    return "\n".join(en for en in [
        f"{chart.name(0)} {chart.number_of_antigens()}:{chart.number_of_sera()} {nprj}",
        fcb,
        projections,
        sources,
        colbases,
        ] if en)

# ======================================================================
