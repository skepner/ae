import sys, datetime
from contextlib import contextmanager

# ======================================================================

@contextmanager
def timeit(name, report=True):
    start = datetime.datetime.utcnow()
    try:
        yield
    except Exception as err:
        print(f">> {name} <{datetime.datetime.utcnow() - start}> with error {err}", file=sys.stderr)
        raise
    else:
        print(f">>> {name} <{datetime.datetime.utcnow() - start}>", file=sys.stderr)

# ======================================================================
