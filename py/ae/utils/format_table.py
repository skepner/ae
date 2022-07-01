import math

# ----------------------------------------------------------------------

class Formatter:

    # do not call it format because str has format method
    def fmt(self, row_no: int, field_no: int, field, width: int):
        if hasattr(field, "fmt"):
            return field.fmt(width)
        elif isinstance(field, int):
            return f"{field:{width}d}"
        elif isinstance(field, float):
            return f"{field:{width}.16f}"
        else:
            return f"{str(field):{width}s}"

class ValueFormatter:

    def __init__(self, value):
        self.value = value

    def width(self) -> int:
        return len(str(self.value))

class Centered (ValueFormatter):

    def fmt(self, width: int, **args) -> str:
        return f"{str(self.value):^{width}s}"

class RightAligned (ValueFormatter):

    def fmt(self, width: int, **args) -> str:
        return f"{str(self.value):>{width}s}"

# ----------------------------------------------------------------------

def format_table(table: list, field_sep: str =" ", formatter: Formatter = None) -> str:
    """formatter is an instance of class derived from Formatter, it may override fmt method.
    """
    if not table:
        return ""
    if isinstance(table[0], list):
        return format_list_of_lists(table, field_sep=field_sep, formatter=formatter)
    else:
        return format_list_of_dicts(table, field_sep=field_sep, formatter=formatter)

# ----------------------------------------------------------------------

def format_list_of_lists(table: list, field_sep: str =" ", formatter: Formatter = None) -> str:

    def calculate_width(field):
        if hasattr(field, "width"):
            return field.width()
        elif isinstance(field, float) and not math.isnan(field):
            return len(str(int(col))) + 17
        else:
            return len(str(field))

    if not formatter:
        formatter = Formatter()
    widths = [0] * len(table[0])
    for row in table:
        for col_no, col in enumerate(row):
            widths[col_no] = max(widths[col_no], calculate_width(col))
    return "\n".join(field_sep.join(formatter.fmt(row_no=row_no, field_no=field_no, field=field, width=widths[field_no]) for field_no, field in enumerate(row)) for row_no, row in enumerate(table))

# ----------------------------------------------------------------------
