import sys

# ======================================================================

def org_table_to_dict(data: str) -> list[dict[str, str]]:
    """only the first table is extrcated, any lines before and after the first table are ignored."""
    result: list[dict[str, str]] = []
    field_names: list[str] = []
    for line in data.split("\n"):
        if line[:2] == "|-":
            pass                # ignore delimiting line
        elif line[:1] == "|":
            fields = [fld.strip() for fld in line.split("|")[1:-1]]
            if not field_names:
                field_names = fields
            else:
                result.append({key: _convert_value(fields[no]) for no, key in enumerate(field_names) if fields[no]})
        elif field_names:
            break
    return result

def _convert_value(value: str) -> object:
    if value in ["true", "True"]:
        return True
    if value in ["false", "False"]:
        return False
    try:
        return int(value)
    except ValueError:
        pass
    try:
        return float(value)
    except ValueError:
        pass
    return value

# ----------------------------------------------------------------------

def dict_to_org_table(data: list[dict[str, object]], field_order: list, add_org_mode_wrapper: bool = True) -> str:
    field_size : dict[str, int] = {field: len(field) for field in field_order}
    for en in data:
        for field, val in en.items():
            field_size[field] = max(field_size.get(field, 0), len(str(val)))
    fields = field_order + [field for field in field_size if field not in field_order]

    res = ""
    if add_org_mode_wrapper:
        res += "# -*- Org -*-\n"
    res += "| " + " | ".join(f"{field:<{field_size[field]}s}" for field in fields) + " |\n" # header
    res += "|" + "+".join(("-" * (field_size[field] + 2)) for field in fields) + "|\n" # separator
    for en in data:
        res += "|"
        for field in fields:
            val = en.get(field, "")
            if isinstance(val, (int, float)):
                res += f" {str(val):>{field_size[field]}s} |"
            else:
                res += f" {str(val):<{field_size[field]}s} |"
        res += "\n"
    if add_org_mode_wrapper:
        res += "# -*-"
    return res

# ======================================================================
