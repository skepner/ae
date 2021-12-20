"""Looks name up in the [geonames database](http://www.geonames.org/).
[GeoNames Search Webservice API](http://www.geonames.org/export/geonames-search.html)
"""

import sys, os, urllib.request, json, time
from pathlib import Path
import logging; module_logger = logging.getLogger(__name__)
from .utilities import is_chinese

# ======================================================================

def geonames(name):
    if not name:
        return name
    if is_chinese(name):
        r = _lookup_chinese(name=name)
    else:
        r = _lookup("search", isNameRequired="true", name=name)
    return r

# ----------------------------------------------------------------------

def _lookup(feature, **args):

    def make(entry):
        if entry.get("fcl") in ["A", "P"]:
            return {
                # "local_name": entry[],
                "name": entry["toponymName"],
                "province": entry["adminName1"],
                "country": entry["countryName"],
                "latitude": entry["lat"],
                "longitude": entry["lng"],
                }
        else:
            return None

    return _get(feature, make, args)

# ----------------------------------------------------------------------

def _get(feature, result_maker, args):
    args.update({"username": "acorg", "type": "json"})
    url = "http://api.geonames.org/{}?{}".format(feature, urllib.parse.urlencode(args))
    # module_logger.debug('_lookup {!r}'.format(url))
    while True:
        rj = json.loads(urllib.request.urlopen(url=url).read().decode("utf-8"))
        try:
            return [e2 for e2 in (result_maker(e1) for e1 in rj["geonames"]) if e2]
        except KeyError:
            if "the hourly limit of" in rj.get("status", {}).get("message"):
                print(f"WARNING: {rj['status']['message']}", file=sys.stderr)
                seconds_to_wait = 120
                print(f"WARNING: about to wait {seconds_to_wait} seconds", file=sys.stderr)
                time.sleep(seconds_to_wait)
            else:
                print(f"ERROR: {rj}", file=sys.stderr)
                raise RuntimeError(str(rj))
        except Exception as err:
            print(f"ERROR: {rj}: {err}", file=sys.stderr)
            raise RuntimeError(f"{rj}: {err}")

# ----------------------------------------------------------------------

def _lookup_chinese(name):
    if len(name) > 3:
        r = []
        if provinces := _find_chinese_province(name):
            province = provinces[0]
            county = _find_chinese_county(name, province);
            if county:
                r = [{
                    "local_name": name,
                    "name": _make_chinese_name(province, county),
                    "province": _make_province_name(province),
                    "country": province["countryName"],
                    "latitude": county["lat"],
                    "longitude": county["lng"],
                    }]
    else:
        def make(entry):
            province_name = _make_province_name(entry)
            return {
                "local_name": name,
                "name": province_name,
                "province": province_name,
                "country": entry["countryName"],
                "latitude": entry["lat"],
                "longitude": entry["lng"],
                }

        r = [make(e) for e in _find_chinese_province(name)]
    return r

# ----------------------------------------------------------------------

def _find_chinese_province(name):
    r = _get("search", lambda e: e if e["name"] == name[:2] else None, {"isNameRequired": "true", "name_startsWith": name[:2], "fclass": "A", "fcode": "ADM1", "lang": "cn"})
    # module_logger.debug('name: {!r} : {!r}'.format(name[:2], r))
    if not r: # Inner Mongolia is written using 3 Hanzi
        r = _get("search", lambda e: e if e["name"] == name[:3] else None, {"isNameRequired": "true", "name_startsWith": name[:3], "fclass": "A", "fcode": "ADM1", "lang": "cn"})
    return r

# ----------------------------------------------------------------------

def _make_province_name(entry):
    r = entry["toponymName"].upper()
    space_pos = r.find(' ', 6 if r[:6] == "INNER " else 0)
    if space_pos >= 0:
        r = r[:space_pos]
    return r;

# ----------------------------------------------------------------------

def _find_chinese_county(full_name, province):
    name = full_name[len(province["name"]):]
    r = _get("search", lambda e: e, {"isNameRequired": "true", "name_startsWith": name, "fclass": "A", "fcode": "ADM3", "adminCode1": province["adminCode1"], "lang": "cn"})
    if not r:
        r = _get("search", lambda e: e, {"isNameRequired": "true", "name_startsWith": name, "adminCode1": province["adminCode1"], "lang": "cn"})
    # module_logger.debug('_find_chinese_county {}'.format(r))
    return r[0] if r else None

# ----------------------------------------------------------------------

def _make_chinese_name(province, county):
    return _make_province_name(province) + " " + _make_county_name(county)

# ----------------------------------------------------------------------

def _make_county_name(county):

    def remove_suffix(source, suffix):
        if source[-len(suffix):] == suffix:
            source = source[:-len(suffix)]
        return source

    def remove_apostrophe(source):
        return source.replace("’", "")

    r = county["toponymName"].upper()
    r1 = remove_suffix(r, " ZIZHIXIAN")
    if r1 != r:
        r = remove_suffix(r1, "ZU")
    else:
        for s in [" QU", " XIAN", " SHI"]:
            r2 = remove_suffix(r, s)
            if r2 != r:
                r = r2
                break
    r = remove_apostrophe(r)
    return r

# ======================================================================
