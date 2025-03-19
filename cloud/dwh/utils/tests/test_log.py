import pytest

from cloud.dwh.utils.log import mask_sensitive_fields


@pytest.mark.parametrize('data, extra_fields, expected', [
    ({}, None, {}),
    ({}, 'b', {}),
    ({}, ['b', 'c'], {}),
    ({'a': '1', 'b': '2', 'c': '3'}, ['a', 'b'], {'a': '[masked value]', 'b': '[masked value]', 'c': '3'}),
    ({'token': 'secret', 'b': 'value'}, None, {'token': '[masked value]', 'b': 'value'}),
    ({'token': 'secret', 'b': 'value'}, 'b', {'token': '[masked value]', 'b': '[masked value]'}),
    ({'nested': {'b': 'secret'}}, 'b', {'nested': {'b': 'secret'}}),
])
def test_mask_sensitive_fields(data, extra_fields, expected):
    unchanged_data = data.copy()

    result = mask_sensitive_fields(data, extra_fields)

    assert result == expected
    assert data == unchanged_data
