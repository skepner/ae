import os
import logging; module_logger = logging.getLogger(__name__)
from . import read

# ----------------------------------------------------------------------

class CannotAdd (Exception): pass

# ======================================================================

def save():
    read.location_db().save()

# ======================================================================

def fix(save=True):
    ldb = read.location_db()
    num_fixes = 0
    for name in ldb.data["locations"]:
        if name not in ldb.data["names"]:
            ldb.data["names"][name] = name
            num_fixes += 1
            module_logger.info('Fixed {!r}'.format(name))
    if num_fixes:
        ldb.updated()
        if save:
            ldb.save()

# ----------------------------------------------------------------------

def add(name, country, division, lat, long, save=True):
    ldb = read.location_db()
    name = name.upper()
    try:
        ldb.find(name=name)
        raise CannotAdd("{!r} already in the database".format(name))
    except read.LocationNotFound:
        pass
    country = country.upper()
    if not ldb.country_exists(country=country):
        raise CannotAdd("Country {!r} unknown".format(country))
    lat = float(lat)
    long = float(long)
    name_lat_long = ldb.find_by_lat_long(lat=lat, long=long)
    if name_lat_long is not None:
        raise CannotAdd("{!r}: Entry with lat/long already exists: {!r}".format(name, name_lat_long))
    division = division.upper()
    ldb.data["locations"][name] = [lat, long, country, division]
    ldb.data["names"][name] = name
    ldb.updated()
    if save:
        ldb.save()

# ----------------------------------------------------------------------

def add_cdc_abbreviation(name, cdc_abbreviation, save=True):
    ldb = read.location_db()
    name = name.upper()
    try:
        ldb.find(name=name)
    except read.LocationNotFound:
        raise CannotAdd("{!r} is not in the database".format(name))
    cdc_abbreviation = cdc_abbreviation.upper()
    if len(cdc_abbreviation) not in [2, 3]:
        raise CannotAdd("cdc abbreviation to add is too short or too long: {!r}".format(cdc_abbreviation))
    if ldb.data["cdc_abbreviations"].get(cdc_abbreviation) is not None:
        raise CannotAdd("cdc abbreviation {!r} already in the database and points to {!r}".format(cdc_abbreviation, ldb.data["cdc_abbreviations"][cdc_abbreviation]))
    for ca, n in ldb.data["cdc_abbreviations"].items():
        if n == name:
            raise CannotAdd("cdc abbreviation {!r} points to the name in te request: {!r}".format(ca, name))
    ldb.data["cdc_abbreviations"][cdc_abbreviation] = name
    ldb.updated()
    if save:
        ldb.save()

# ----------------------------------------------------------------------

def add_new_name(name, new_name, save=True):
    ldb = read.location_db()
    name = name.upper()
    try:
        entry = ldb.find(name=name)
    except read.LocationNotFound:
        raise CannotAdd("{!r} is not in the database".format(name))
    new_name = new_name.upper()
    try:
        ldb.find(name=new_name)
        raise CannotAdd("{!r} already in the database".format(new_name))
    except read.LocationNotFound:
        pass
    ldb.data["names"][new_name] = entry["found"]
    ldb.updated()
    if save:
        ldb.save()

# ----------------------------------------------------------------------

def add_replacement(name_to_replace_with, new_name, save=True):
    ldb = read.location_db()
    name_to_replace_with = name_to_replace_with.upper()
    try:
        ldb.find(name=name_to_replace_with)
    except read.LocationNotFound:
        raise CannotAdd("{!r} is not in the database".format(name_to_replace_with))
    new_name = new_name.upper()
    try:
        ldb.find(name=new_name)
        raise CannotAdd("{!r} already in the database".format(new_name))
    except read.LocationNotFound:
        pass
    except read.LocationReplacement:
        raise CannotAdd("{!r} already in the database".format(new_name))
    ldb.data["replacements"][new_name] = name_to_replace_with
    ldb.updated()
    if save:
        ldb.save()

# ======================================================================
