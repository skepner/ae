"""
Support for directory modules (ae.py)
"""

from pathlib import Path
import importlib.util

# ======================================================================

sLoaded = {}

# ======================================================================

def load(dir: Path):
    if mod := sLoaded.get(dir):
        return mod
    spec = importlib.util.spec_from_file_location("ae", dir.joinpath("ae.py"))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    if init := getattr(mod, "init"):
        init()
    sLoaded[dir] = mod
    return mod

# ======================================================================
