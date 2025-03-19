
import pytest

from typing import List

from yc_common.numeric_range_allocator import NumericRangeDescriptor, get_overlapping_ranges, NumericRangeAllocator
from yc_common.clients.kikimr.sql import build_query_template


class MockKikimrSpec():
    def __init__(self, primary_keys):
        self.primary_keys = primary_keys


class MockKikiTable():
    def __init__(self, primary_keys):
        self.spec = MockKikimrSpec(primary_keys)

    def current_spec(self):
        return self.spec


def raw_to_range_list(ranges: List[dict]) -> List[NumericRangeDescriptor]:
    """ Convert list of dict to list of models"""
    return [NumericRangeDescriptor(x) for x in ranges]


def get_range_keys(is_prefix_prime) -> List[str]:
    result = ["resource_id", "start"]
    if is_prefix_prime:
        result += ["prefix"]
    return result


def get_elems_keys(is_prefix_prime) -> List[str]:
    result = ["resource_id", "element"]
    if is_prefix_prime:
        result += ["prefix"]
    return result


def get_test_allocator(is_prefix_prime):
    return NumericRangeAllocator(MockKikiTable(get_range_keys(is_prefix_prime)),
                                 MockKikiTable(get_elems_keys(is_prefix_prime)), 'test-allocator')


@pytest.mark.parametrize("range_list, expected_list", [
    # No overlaps
    (({"start": 1, "end": 2, "prefix": 0}, {"start": 3, "end": 4, "prefix": 0}), []),
    # Total overlap
    (({"start": 1, "end": 2, "prefix": 0}, {"start": 1, "end": 4, "prefix": 0}),
        ({"start": 1, "end": 2, "prefix": 0}, {"start": 1, "end": 4, "prefix": 0})),
    (({"start": 1, "end": 2, "prefix": 0}, {"start": 1, "end": 4, "prefix": 1}), []),
    # result is sorted
    (({"start": 2, "end": 4, "prefix": 0}, {"start": 1, "end": 4, "prefix": 0}),
        ({"start": 1, "end": 4, "prefix": 0}, {"start": 2, "end": 4, "prefix": 0})),
    # same is overlap
    (({"start": 1, "end": 4, "prefix": 0}, {"start": 1, "end": 4, "prefix": 0}),
        ({"start": 1, "end": 4, "prefix": 0}, {"start": 1, "end": 4, "prefix": 0})),
    # Partial overlap
    (({"start": 1, "end": 2, "prefix": 0}, {"start": 1, "end": 4, "prefix": 0}, {"start": 10, "end": 20, "prefix": 0}),
        ({"start": 1, "end": 2, "prefix": 0}, {"start": 1, "end": 4, "prefix": 0})),
    # Prefix is significant, at least for now.
    (({"start": 1, "end": 2, "prefix": 0}, {"start": 1, "end": 2, "prefix": 1}, {"start": 0, "end": 3, "prefix": 2}), []),


])
def test_allocator_overlaps_ranges(range_list, expected_list):
    to_test = raw_to_range_list(range_list)
    to_expect = raw_to_range_list(expected_list)
    result = get_overlapping_ranges(to_test)
    assert result == to_expect


@pytest.mark.parametrize("range_list, is_prefixed, expected_expression", [

    # one element
    (({"start": 1, "end": 2, "prefix": 0}, ), False, "FALSE OR element BETWEEN 1 AND 2"),
    (({"start": 1, "end": 2, "prefix": 0}, ), True, "FALSE OR element BETWEEN 1 AND 2 AND prefix = 0"),
    # Two elements
    (({"start": 1, "end": 2, "prefix": 0}, {"start": 10, "end": 20, "prefix": 1}, ), False,
       "FALSE OR element BETWEEN 1 AND 2 OR element BETWEEN 10 AND 20"),
    (({"start": 1, "end": 2, "prefix": 0}, {"start": 10, "end": 20, "prefix": 1}, ), True,
       "FALSE OR element BETWEEN 1 AND 2 AND prefix = 0 OR element BETWEEN 10 AND 20 AND prefix = 1"),

])
def test_allocator_kiki_request_elems_no_prefix(range_list, is_prefixed, expected_expression):
    test_allocator = get_test_allocator(is_prefixed)
    to_test = raw_to_range_list(range_list)
    condition = test_allocator._get_where_condition_from_ranges_elems(to_test)
    q_tmpl = build_query_template("?", condition)
    assert is_prefixed == test_allocator._is_prefix_primary_alloc()
    assert q_tmpl.build_text_query() == expected_expression


# TODO: use SqlIn after KIKIMR-5598
@pytest.mark.parametrize("range_list, is_prefixed, expected_expression", [
    # one element
    (({"start": 1, "end": 2, "prefix": 0}, ), False, "(start = 1)"),
    (({"start": 1, "end": 2, "prefix": 0}, ), True, "(prefix = 0 AND start = 1)"),
    # two elements
    (({"start": 1, "end": 2, "prefix": 0}, {"start": 10, "end": 20, "prefix": 1}, ), False,
       "(start = 1 OR start = 10)"),
    (({"start": 1, "end": 2, "prefix": 0}, {"start": 10, "end": 20, "prefix": 1}, ), True,
       "(prefix = 0 AND start = 1 OR prefix = 1 AND start = 10)"),

])
def test_allocator_kiki_request_range(range_list, is_prefixed, expected_expression):
    test_allocator = get_test_allocator(is_prefixed)
    to_test = raw_to_range_list(range_list)
    condition = test_allocator._get_where_condition_from_ranges(to_test)
    q_tmpl = build_query_template("?", condition)
    assert is_prefixed == test_allocator._is_prefix_primary_range()
    assert q_tmpl.build_text_query() == expected_expression
