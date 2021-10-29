"""
Support for directory modules (ae.py)
"""

from pathlib import Path
import importlib.util

# ======================================================================

sLoaded = {}

# ======================================================================

def load(dir: Path):
    try:
        return sLoaded[dir]
    except KeyError:
        mod_filename = dir.joinpath("ae.py")
        if mod_filename.exists():
            spec = importlib.util.spec_from_file_location("ae", mod_filename)
            mod = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(mod)
            sLoaded[dir] = mod
            return mod
        else:
            print(f">> no {mod_filename}")
            sLoaded[dir] = None
            return None

# ======================================================================
