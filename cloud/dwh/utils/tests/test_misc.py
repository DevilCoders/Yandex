import pytest

from cloud.dwh.utils.misc import drop_none
from cloud.dwh.utils.misc import ellipsis_string
from cloud.dwh.utils.misc import format_url


@pytest.mark.parametrize('d, none, expected', [
    ({}, None, {}),
    ({'a': None}, None, {}),
    ({'a': 1}, None, {'a': 1}),
    ({'a': 1, 'b': None}, None, {'a': 1}),
    ({'a': 1, 'b': {'c': None}}, None, {'a': 1, 'b': {'c': None}}),
    ({'a': 1, 'b': None}, 1, {'b': None}),

])
def test_drop_none(d, none, expected):
    unchanged_d = d.copy()

    result = drop_none(d, none)

    assert result == expected
    assert d == unchanged_d


@pytest.mark.parametrize('url, query_params, expected', [
    ('https://example.com', None, 'https://example.com'),
    ('https://example.com', {}, 'https://example.com'),
    ('https://example.com/a/b', {'q': 'val'}, 'https://example.com/a/b?q=val'),
    ('https://example.com/a/b?', {'q': 'val'}, 'https://example.com/a/b?q=val'),
    ('https://example.com/a/b?q=val', {'q': 'val2'}, 'https://example.com/a/b?q=val&q=val2'),
    ('https://example.com/a/b?q=val', {}, 'https://example.com/a/b?q=val'),
    ('https://example.com/a/b?q=val&q=val2', {'a': 'v', 'b': 'v2'}, 'https://example.com/a/b?q=val&q=val2&a=v&b=v2'),
])
def test_format_url(url, query_params, expected):
    result = format_url(url, query_params=query_params)

    assert result == expected


@pytest.mark.parametrize('s, max_length, expected', [
    ('1234567890', 1, '>...<'),
    ('1234567890', 5, '>...<'),
    ('1234567890', 6, '>...<'),
    ('1234567890', 7, '1>...<0'),
    ('1234567890', 8, '1>...<0'),
    ('1234567890', 9, '12>...<90'),
    ('1234567890', 10, '1234567890'),
    ('1234567890', 20, '1234567890'),
])
def test_ellipsis_string(s, max_length, expected):
    result = ellipsis_string(s, max_length)

    assert result == expected
