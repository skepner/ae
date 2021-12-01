from pathlib import Path

# ======================================================================

def open_file(path :Path):
    import lzma, bz2, gzip
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

# ======================================================================
