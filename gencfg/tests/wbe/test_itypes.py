"""Tests itypes."""

from __future__ import unicode_literals

import os

from tests.util import get_yaml, get_db_by_path

import pytest

unstable_only = pytest.mark.unstable_only


@unstable_only
def test_create_and_remove(wbe):
    try:
        itype = _create_itype(wbe)
        _remove_itype(wbe, itype)
    finally:
        try:
            _remove_itype(wbe, itype)
        except:
            pass


def test_get(wbe):
    itype_name = sorted(wbe.api.get("/unstable/itypes")["itypes"])[0]["itype"]

    result = wbe.api.get("/unstable/itypes/%s" % itype_name)

    db = get_db_by_path(wbe.db_path, cached=False)

    assert db.itypes.has_itype(itype_name)
    assert db.itypes.get_itype(itype_name).descr == result["descr"]


def test_get_all(wbe):
    wbe.api.get("/unstable/itypes")["itypes"]


@unstable_only
def test_rename(wbe):
    itype = _create_itype(wbe)

    new_name = "TestRenamedItypes"
    _check_no_itype(wbe, new_name)

    wbe.api.check_response_status(wbe.api.post("/unstable/itypes", {
        "action": "rename",
        "itype": itype["name"],
        "new_itype": new_name,
    }))
    _check_no_itype(wbe, itype["name"])
    itype["name"] = new_name

    _check_itype(wbe, itype["name"])
    _remove_itype(wbe, itype)


@unstable_only
def test_remove_missing(wbe):
    name = "TestItype"
    _check_no_itype(wbe, name)

    wbe.api.check_response_status(wbe.api.post("/unstable/itypes", {
        "action": "remove",
        "itype": name,
    }), ok=False)

    _check_no_itype(wbe, name)


def _create_itype(wbe):
    itype = {
        "name": "TestItype",
        "descr": "Sample description",
    }

    _check_no_itype(wbe, itype["name"])

    wbe.api.check_response_status(wbe.api.post("/unstable/itypes", {
        "action": "add",
        "itype": itype["name"],
        "descr": itype["descr"],
    }))

    _check_itype(wbe, itype["name"])

    return itype


def _remove_itype(wbe, itype):
    wbe.api.check_response_status(wbe.api.post("/unstable/itypes", {
        "action": "remove",
        "itype": itype["name"],
    }))

    _check_no_itype(wbe, itype["name"])


def _check_no_itype(wbe, name):
    assert _get_itype(wbe, name) is None


def _check_itype(wbe, itype_name):
    assert _get_itype(wbe, itype_name)["name"] == itype_name


def _get_itype(wbe, name):
    for itype in _get_itypes(wbe):
        if itype["name"] == name:
            return itype


def _get_itypes(wbe):
    return get_yaml(os.path.join(wbe.db_path, "itypes.yaml"))
