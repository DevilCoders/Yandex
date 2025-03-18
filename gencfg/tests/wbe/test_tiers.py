"""Tests tiers."""

from __future__ import unicode_literals

import os

from tests.util import get_yaml

import pytest

unstable_only = pytest.mark.unstable_only


@unstable_only
def test_create_and_remove(wbe):
    tier = _create_tier(wbe)
    _remove_tier(wbe, tier)


def test_get(wbe):
    tier_name = sorted(wbe.api.get("/unstable/tiers")["tiers"])[0]["tier"]
    assert wbe.api.get("/unstable/tiers/{}".format(tier_name))["tier"] == tier_name


def test_get_all(wbe):
    wbe.api.get("/unstable/tiers")["tiers"]


@unstable_only
def test_rename(wbe):
    tier = _create_tier(wbe)

    new_name = "TestRenamedTier"
    _check_no_tier(wbe, new_name)

    wbe.api.check_response_status(wbe.api.post("/unstable/tiers", {
        "action": "rename",
        "tier": tier["name"],
        "new_tier": new_name,
    }))
    _check_no_tier(wbe, tier["name"])
    tier["name"] = new_name

    _check_tier(wbe, tier)
    _remove_tier(wbe, tier)


@unstable_only
def test_remove_missing(wbe):
    name = "TestTier"
    _check_no_tier(wbe, name)

    wbe.api.check_response_status(wbe.api.post("/unstable/tiers", {
        "action": "remove",
        "tier": name,
    }), ok=False)

    _check_no_tier(wbe, name)


def _create_tier(wbe):
    tier = {
        "name": "TestTier",
        "primuses": [{"name": "test_primus", "shards": range(10)}],
    }

    _check_no_tier(wbe, tier["name"])

    wbe.api.check_response_status(wbe.api.post("/unstable/tiers", {
        "action": "add",
        "tier": tier["name"],
        "primus": tier["primuses"][0]["name"],
        "shards_count": len(tier["primuses"][0]["shards"]),
    }))

    _check_tier(wbe, tier)

    return tier


def _remove_tier(wbe, tier):
    wbe.api.check_response_status(wbe.api.post("/unstable/tiers", {
        "action": "remove",
        "tier": tier["name"],
    }))

    _check_no_tier(wbe, tier["name"])


def _check_no_tier(wbe, name):
    assert _get_tier(wbe, name) is None


def _check_tier(wbe, tier):
    tier_name = tier["name"]
    assert _get_tier(wbe, tier_name)["name"] == tier_name


def _get_tier(wbe, name):
    for tier in _get_tiers(wbe):
        if tier["name"] == name:
            return tier


def _get_tiers(wbe):
    return get_yaml(os.path.join(wbe.db_path, "tiers.yaml"))
