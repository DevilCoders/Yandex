"""
Tests for helpers module.
"""

from dbaas_internal_api.utils.helpers import coalesce


def test_coalesce():
    assert coalesce(True, False) is True
    assert coalesce(None, False) is False
    assert coalesce(False, None) is False
    assert coalesce(True) is True
    assert coalesce(None) is None
    assert coalesce() is None
