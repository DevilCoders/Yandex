"""Tests other stuff."""

from __future__ import unicode_literals

import os
import copy

import pytest

unstable_only = pytest.mark.unstable_only


def test_check(wbe):
    [wbe.api.get("/unstable/check")]


def test_get_all_tags(wbe):
    response = wbe.api.get("/unstable/tags")

    print response
