import sys, traceback, subprocess

# ======================================================================

def format_exception(err: Exception):
    tb = traceback.format_exception(err)
    tb[-1] = "> " + tb[-1]
    return "".join(tb)

def report_exception(err: Exception = None, sound: bool = True):
    if err is None:
        err = sys.exc_info()[1]
    print(format_exception(err), file=sys.stderr)
    if sound:
        subprocess.call(["afplay", "/System/Library/Sounds/Blow.aiff"])

# ======================================================================
