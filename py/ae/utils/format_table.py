import math

# ----------------------------------------------------------------------

def format_table(table: list, field_sep: str =" "):
    if not table:
        return ""
    if isinstance(table[0], list):
        return format_list_of_lists(table, field_sep=field_sep)
    else:
        return format_list_of_dicts(table, field_sep=field_sep)

# ----------------------------------------------------------------------

def format_list_of_lists(table: list, field_sep: str =" "):
    widths = [0] * len(table[0])
    for row in table:
        for col_no, col in enumerate(row):
            if isinstance(col, float) and not math.isnan(col):
                widths[col_no] = max(widths[col_no], len(str(int(col))))
            else:
                widths[col_no] = max(widths[col_no], len(str(col)))
    return "\n".join(field_sep.join(format_field(field, widths[field_no]) for field_no, field in enumerate(row)) for row in table)

# ----------------------------------------------------------------------

def format_field(field, width: int):
    if isinstance(field, int):
        return f"{field:{width}d}"
    elif isinstance(field, float):
        return f"{field:{width + 17}.16f}"
    else:
        return f"{str(field):{width}s}"

# ----------------------------------------------------------------------