import os, datetime, pprint
from pathlib import Path
import logging; module_logger = logging.getLogger(__name__)
from .utilities import timeit, read_json, write_json

# ======================================================================

class LocationNotFound (Exception): pass

class CountryNotFound (Exception): pass

class LocationReplacement (Exception):
    """Raised when replacement is found."""

    def __init__(self, replacement):
        super().__init__()
        self.replacement = replacement

    def __str__(self):
        return 'Replace with ' + self.replacement

# ======================================================================

def find(name, like=False, handle_replacement=False):
    """May raise LocationNotFound and LocationReplacement"""
    entry = location_db().find(name=name, like=like, handle_replacement=handle_replacement)
    if isinstance(entry, list):
        return entry
    else:
        return [entry]

# ======================================================================

def find_cdc_abbreviation(cdc_abbreviation):
    return location_db().find_cdc_abbreviation(cdc_abbreviation=cdc_abbreviation)

# ======================================================================

def find_cdc_abbreviation_for_name(name):
    return location_db().find_cdc_abbreviation_for_name(name)

# ======================================================================

def country(name):
    """Returns country by location name. May raise LocationNotFound
    and LocationReplacement"""
    return location_db().find(name=name)["country"]

# ======================================================================

def continent(name=None):
    """Returns continent by either location name or country. May raise
    LocationNotFound and LocationReplacement"""
    ldb = location_db()
    try:
        return ldb.find_continent(country=name)
    except LocationNotFound:
        return ldb.find_continent(country=ldb.find(name=name)["country"])

# ----------------------------------------------------------------------

def check():
    ldb = location_db()
    try:
        ldb.check()
    except Exception as err:
        module_logger.error(err)

# ======================================================================

class LocationDb:

    class LocationEntry (dict):

        def __getattr__(self, name):
            try:
                return self[name]
            except KeyError:
                raise AttributeError(name)

    def __init__(self):
        with timeit("Loading LocationDb"):
            self.dbfile = Path(os.environ["LOCDB_V2"])
            if not self.dbfile.exists():
                raise RuntimeError(f"LocationDb file not found (LOCDB_V2={os.environ.get('LOCDB_V2')})")
            self.data = read_json(self.dbfile)
            try:
                del self.data["_"]
            except:
                pass
            self._updated = False

    # "?continents": "[continent]",
    # "?countries": "{country: index in continents}",
    # "?locations": "{name: [latitude, longitude, country, division]}",
    # "?names": "{name: name in locations}",
    # "?cdc_abbreviations": "{cdc_abbreviation: name in locations}",
    # "?replacements": "{name: name in names}",

    def find(self, name, like=False, handle_replacement=False):
        name = name.upper()
        ns = name.strip()
        if ns != name:
            name = replacement = ns
        else:
            replacement = None
        try:
            n = self.data["names"][name]
        except KeyError:
            try:
                replacement = self.data["replacements"][name]
                if not handle_replacement:
                    raise LocationReplacement(replacement)
                n = self.data["names"][replacement]
            except KeyError:
                n = name
        try:
            r = self._make_result(name=name, found=n, loc=self.data["locations"][n], replacement=replacement)
        except KeyError:
            if like:
                r = self.find_like(name)
            else:
                raise LocationNotFound(name)
        return r

    def find_cdc_abbreviation(self, cdc_abbreviation):
        try:
            return self.find(self.data["cdc_abbreviations"][cdc_abbreviation.upper()])
        except KeyError:
            raise LocationNotFound(cdc_abbreviation)

    def find_cdc_abbreviation_for_name(self, name):
        abbr = None
        try:
            location_name = self.find(name)["found"]
            for ca, loc in self.data["cdc_abbreviations"].items():
                if loc == location_name:
                    abbr = ca
                    break
        except LocationNotFound:
            pass
        return abbr

    def find_continent(self, country):
        try:
            return self.data["continents"][self.data["countries"][country]]
        except:
            raise LocationNotFound(country)

    def _make_result(self, name, found, loc, replacement=None):
        r = self.LocationEntry({"name": name, "found": found, "latitude": loc[0], "longitude": loc[1], "country": loc[2], "division": loc[3]})
        try:
            r["continent"] = self.data["continents"][self.data["countries"][loc[2]]]
        except:
            module_logger.error('No continent for {}'.format(loc[2]))
            r["continent"] = "UNKNOWN"
        if replacement:
            r["replacement"] = replacement
        return r

    def find_like(self, name):
        return [self._make_result(name=n, found=nn, loc=self.data["locations"][nn]) for n, nn in self.data["names"].items() if name in n]

    def country_exists(self, country):
        return self.data["countries"].get(country) is not None

    def find_by_lat_long(self, lat, long):
        for n, e in self.data["locations"].items():
            if abs(e[0] - lat) < 0.01 and abs(e[1] - long) < 0.01:
                return n
        return None

    def version(self):
        return self.data["  version"] + "-" + self.data[" date"]

    def updated(self):
        self.data[" date"] = datetime.date.today().strftime("%Y-%m-%d")
        self._updated = True

    def check(self):
        """Check if every "locations" entry has corresponding "names" entry"""
        missing = []
        for name in self.data["locations"]:
            try:
                n = self.data["names"][name]
            except KeyError:
                missing.append(name)
        if missing:
            raise RuntimeError("\"names\" list lacks:\n  " + "\n  ".join(missing))

    def save(self):
        if self._updated:
            write_json(self.dbfile, self.data, indent=1, sort_keys=True, backup=True)
            self._updated = False

# ======================================================================

class LocationDbNotFound:

    def find(self, name, like=False, **kwargs):
        module_logger.warning('Trying to find location {!r} in LocationDbNotFound'.format(name))
        raise LocationNotFound('LocationDb not available')

# ======================================================================

sLocationDb = None  # singleton

def location_db():
    global sLocationDb
    if sLocationDb is None:
        if not os.environ.get("LOCDB_V2"):
            raise RuntimeError("LOCDB_V2 not set")
        try:
            sLocationDb = LocationDb()
        except Exception as err:
            module_logger.error('LocationDb creation failed: {}'.format(err))
            sLocationDb = LocationDbNotFound()
    return sLocationDb

# ======================================================================
