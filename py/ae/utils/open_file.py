import sys, lzma, bz2, gzip, datetime
from pathlib import Path

# ======================================================================

def for_reading(path :Path):
    if path == "-":
        return sys.stdin
    for opener in [lzma.LZMAFile, bz2.BZ2File, gzip.GzipFile, open]:
        file = opener(path, "rb")
        try:
            file.peek(1)
            break
        except:
            pass
    else:
        raise RuntimeError(f"Cannot read {path}")
    return file

# ----------------------------------------------------------------------

def for_writing(path :Path, do_backup: bool = True):
    if path == "-":
        return sys.stdout
    if do_backup:
        backup(path)
    if path.suffix in [".xz", ".ace", ".jxz", ".tjz"]:
        return lzma.LZMAFile(path, "w")
    elif path.suffix in [".bz2"]:
        return lzma.BZ2File(path, "w")
    elif path.suffix in [".gz"]:
        return gzip.GzipFile(path, "w")
    else:
        return path.open("w")

# ----------------------------------------------------------------------

def backup(path: Path):
    if path.exists() and path.parents[-1] != "dev":
        backup_dir = path.resolve().parent.joinpath(".backup")
        backup_dir.mkdir(parents=True, exist_ok=True)
        now = datetime.datetime.now()
        new_name = backup_dir.joinpath(f"{path.stem}.~{now:%Y-%m%d-%H%M%S}~.{path.suffix}")
        if not new_name.exists():
            path.rename(new_name)

# ======================================================================
