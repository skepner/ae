from pathlib import Path
import importlib.util, importlib.machinery

# ----------------------------------------------------------------------

sLoaded = {}

def load(module_file: Path|str):
    module_file = Path(module_file).resolve()
    try:
        return sLoaded[module_file]
    except KeyError:
        if module_file.exists():
            if spec := importlib.util.spec_from_file_location("ae", module_file, loader=importlib.machinery.SourceFileLoader("ae", str(module_file))):
                mod = importlib.util.module_from_spec(spec)
                spec.loader.exec_module(mod)
                sLoaded[module_file] = mod
                return mod
            else:
                raise RuntimeError(f"importlib.util.spec_from_file_location failed for \"{module_file}\"")
        else:
            print(f">> no {module_file}")
            sLoaded[module_file] = None
            return None

# ----------------------------------------------------------------------
