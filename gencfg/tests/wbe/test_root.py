"""Tests root page."""

from __future__ import unicode_literals

import pytest

unstable_only = pytest.mark.unstable_only


@unstable_only
def test_root(wbe):
    result = wbe.api.get("/unstable/").keys()
    assert ("groups" in result)
    assert ("intlookups" in result)
    assert ("tiers" in result)
