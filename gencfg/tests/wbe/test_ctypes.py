"""Tests ctypes."""

from __future__ import unicode_literals

import os

from tests.util import get_yaml, get_db_by_path

import pytest

unstable_only = pytest.mark.unstable_only


@unstable_only
def test_create_and_remove(wbe):
    try:
        ctype = _create_ctype(wbe)
        _remove_ctype(wbe, ctype)
    finally:
        try:
            _remove_ctype(wbe, ctype)
        except:
            pass


def test_get(wbe):
    ctype_name = sorted(wbe.api.get("/unstable/ctypes")["ctypes"])[0]["ctype"]

    result = wbe.api.get("/unstable/ctypes/%s" % ctype_name)

    db = get_db_by_path(wbe.db_path, cached=False)

    assert db.ctypes.has_ctype(ctype_name)
    assert db.ctypes.get_ctype(ctype_name).descr == result["descr"]


def test_get_all(wbe):
    wbe.api.get("/unstable/ctypes")["ctypes"]


@unstable_only
def test_rename(wbe):
    try:
        ctype = _create_ctype(wbe)

        new_name = "TestRenamedCtypes"
        _check_no_ctype(wbe, new_name)

        wbe.api.check_response_status(wbe.api.post("/unstable/ctypes", {
            "action": "rename",
            "ctype": ctype["ctype"],
            "new_ctype": new_name,
        }))
        _check_no_ctype(wbe, ctype["ctype"])
        ctype["ctype"] = new_name

        _check_ctype(wbe, ctype["ctype"])
    finally:
        _remove_ctype(wbe, ctype)


@unstable_only
def test_remove_missing(wbe):
    name = "TestCtype"
    _check_no_ctype(wbe, name)

    wbe.api.check_response_status(wbe.api.post("/unstable/ctypes", {
        "action": "remove",
        "ctype": name,
    }), ok=False)

    _check_no_ctype(wbe, name)


def _create_ctype(wbe):
    ctype = {
        "ctype": "TestCtype",
        "descr": "Sample description",
    }

    _check_no_ctype(wbe, ctype["ctype"])

    wbe.api.check_response_status(wbe.api.post("/unstable/ctypes", {
        "action": "add",
        "ctype": ctype["ctype"],
        "descr": ctype["descr"],
    }))

    _check_ctype(wbe, ctype["ctype"])

    return ctype


def _remove_ctype(wbe, ctype):
    wbe.api.check_response_status(wbe.api.post("/unstable/ctypes", {
        "action": "remove",
        "ctype": ctype["ctype"],
    }))

    _check_no_ctype(wbe, ctype["ctype"])


def _check_no_ctype(wbe, name):
    assert _get_ctype(wbe, name) is None


def _check_ctype(wbe, ctype_name):
    assert _get_ctype(wbe, ctype_name)["name"] == ctype_name


def _get_ctype(wbe, name):
    for ctype in _get_ctypes(wbe):
        if ctype["name"] == name:
            return ctype


def _get_ctypes(wbe):
    return get_yaml(os.path.join(wbe.db_path, "ctypes.yaml"))
