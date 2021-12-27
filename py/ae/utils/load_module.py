from pathlib import Path
import importlib.util

# ----------------------------------------------------------------------

sLoaded = {}

def load(module_file: Path):
    try:
        return sLoaded[module_file]
    except KeyError:
        if module_file.exists():
            spec = importlib.util.spec_from_file_location("ae", module_file)
            mod = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(mod)
            sLoaded[module_file] = mod
            return mod
        else:
            print(f">> no {module_file}")
            sLoaded[module_file] = None
            return None

# ----------------------------------------------------------------------
