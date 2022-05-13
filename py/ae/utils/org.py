
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
                result.append({key: fields[no] for no, key in enumerate(field_names) if fields[no]})
        elif field_names:
            break
    return result

# ======================================================================
