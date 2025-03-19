"""
Tests for additional Hamcrest matchers.
"""
import pytest
from hamcrest import assert_that, contains

from tests.helpers.matchers import is_subset_of


class TestIsSubsetOfMatcher:
    def test_string_match(self):
        assert_that("123", is_subset_of("123"))

    def test_string_mismatch(self):
        with pytest.raises(AssertionError):
            assert_that("12", is_subset_of("123"))

    def test_int_match(self):
        assert_that(123, is_subset_of(123))

    def test_int_mismatch(self):
        with pytest.raises(AssertionError):
            assert_that(12, is_subset_of(123))

    def test_bool_match(self):
        assert_that(True, is_subset_of(True))

    def test_bool_mismatch(self):
        with pytest.raises(AssertionError):
            assert_that(True, is_subset_of(False))

    def test_none_match(self):
        assert_that(None, is_subset_of(None))

    def test_none_mismatch(self):
        with pytest.raises(AssertionError):
            assert_that(None, is_subset_of([]))

    def test_list_match(self):
        assert_that([1], is_subset_of([1, 2]))

    def test_list_mismatch(self):
        with pytest.raises(AssertionError):
            assert_that([1, 2], is_subset_of([1, 3]))

    def test_tuple_match(self):
        assert_that((1,), is_subset_of((1, 2)))

    def test_tuple_mismatch(self):
        with pytest.raises(AssertionError):
            assert_that((1, 2), is_subset_of((1, 3)))

    def test_dict_match(self):
        assert_that({'a': 1}, is_subset_of({'a': 1, 'b': 2}))

    def test_dict_mismatch(self):
        with pytest.raises(AssertionError):
            assert_that({'a': 1, 'c': 3}, is_subset_of({'a': 1, 'b': 2}))

    def test_list_mismatch_dict(self):
        with pytest.raises(AssertionError):
            assert_that(['a', 'b'], is_subset_of({'a': 1, 'b': 2}))

    def test_dict_mismatch_list(self):
        with pytest.raises(AssertionError):
            assert_that({'a': 1, 'b': 2}, is_subset_of(['a', 'b']))


class TestIsFullSubsetOfMatcher:
    def test_list_match(self):
        assert_that([1, 2], is_subset_of([2, 1], full=True))

    def test_list_superset_mismatch(self):
        with pytest.raises(AssertionError):
            assert_that([1, 2], is_subset_of([1], full=True))

    def test_list_subset_mismatch(self):
        with pytest.raises(AssertionError):
            assert_that([1], is_subset_of([1, 2], full=True))

    def test_map_match(self):
        assert_that({'a': 1, 'b': 2}, is_subset_of({'b': 2, 'a': 1}, full=True))

    def test_map_superset_mismatch(self):
        with pytest.raises(AssertionError):
            assert_that({'a': 1, 'b': 2}, is_subset_of({'a': 1}, full=True))

    def test_map_subset_mismatch(self):
        with pytest.raises(AssertionError):
            assert_that({'a': 1}, is_subset_of({'a': 1, 'b': 2}, full=True))
