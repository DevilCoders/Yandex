#!/usr/bin/env python
# -*- coding: utf-8 -*-

from hamcrest import assert_that, equal_to, has_key, has_length, instance_of
from hamcrest.core.base_matcher import BaseMatcher


def iterable_contains(expected):
    return IterableContains(expected)


class IterableContains(BaseMatcher):
    def __init__(self, expected):
        assert_that(expected, instance_of((list, dict)))
        self.expected = expected

    def _matches(self, real):
        _compare(self.expected, real)
        return True

    def describe_to(self, _description):
        pass


def _compare(expected, real):
    if isinstance(expected, dict):
        _compare_dicts(expected, real)
    elif isinstance(expected, list):
        _compare_lists(expected, real)
    else:
        assert_that(real, equal_to(expected))


def _compare_dicts(expected, real):
    for key in expected:
        assert_that(real, has_key(key))
        _compare(expected[key], real[key])


def _compare_lists(expected, real):
    """
    Checks lists in the same order
    """
    assert_that(real, has_length(len(expected)))
    for i, expected_item in enumerate(expected):
        _compare(expected_item, real[i])
