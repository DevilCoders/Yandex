"""Tests intlookups."""

from __future__ import unicode_literals

import os

from tests.util import get_yaml, get_db_by_path

import pytest

unstable_only = pytest.mark.unstable_only


def test_get(wbe):
    intlookup_name = wbe.api.get("/unstable/intlookups")["intlookups"][0]["intlookup"]

    response = wbe.api.get("/unstable/intlookups/%s" % intlookup_name)

    assert response["name"] == intlookup_name
    assert response["summary"][0]["key"] == "Intlookup name"
    assert response["summary"][0]["value"]["value"] == intlookup_name

    db_intlookup = get_db_by_path(wbe.db_path, cached=False).intlookups.get_intlookup(str(intlookup_name))
    assert response["summary"][1]["key"] == "Basesearchers type"
    assert response["summary"][1]["value"]["value"] == db_intlookup.base_type


def test_get_all(wbe):
    from_api = wbe.api.get("/unstable/intlookups")["intlookups"]
    from_api = set(map(lambda x: x["intlookup"], from_api))

    db = get_db_by_path(wbe.db_path, cached=False)
    from_db = set(map(lambda x: x.file_name, db.intlookups.get_intlookups()))

    assert from_api == from_db, "Intlookups, got from api and from db differs"
