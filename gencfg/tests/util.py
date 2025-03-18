"""Contains various test utils."""

from __future__ import unicode_literals

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import json

from pkg_resources import require

require('PyYAML')
import yaml

"""
    Useful decorators
"""


def static_var(varname, value):
    def decorate(func):
        setattr(func, varname, value)
        return func

    return decorate


def get_contents(path):
    with open(path) as f:
        return f.read()


def get_yaml(path):
    with open(path) as f:
        return yaml.load(f)


def get_json(path):
    with open(path) as f:
        return json.load(f)


@static_var("db", None)
def get_db_by_path(path, cached=True):
    import gencfg
    from core.db import DB

    if cached == True and get_db_by_path.db is not None:
        return get_db_by_path.db

    retdb = DB(path)
    get_db_by_path.db = retdb
    return retdb


def dbpath_get_group(wbe, groupname, cached=True):
    db = get_db_by_path(wbe.db_path, cached=cached)
    return db.groups.get_group(groupname)


def dbpath_has_group(wbe, groupname, cached=True):
    db = get_db_by_path(wbe.db_path, cached=cached)
    return db.groups.has_group(groupname)
