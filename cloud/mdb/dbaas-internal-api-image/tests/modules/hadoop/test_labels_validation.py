'''
Test for validation Hadoop instances lables
'''
import string

import pytest

from dbaas_internal_api.core.exceptions import DbaasClientError
from dbaas_internal_api.modules.hadoop.validation import validate_labels


def test_validate_correct_labels():
    labels = {
        'subcid1': {
            'cloud_id': 'e4u7k9nq6phhs5jk412c',
            'cluster_id': 'e4u7k9nq6phhs5jk422c',
            'subcluster_id': 'e4u7k9nq6phhs5jk432c',
            'subcluster_role': 'e4u7k9nq6phhs5jk442c',
            'foo': 'bar',
        },
    }
    validate_labels(labels)


def test_validate_correct_empty_labels():
    labels = {}
    validate_labels(labels)


def test_validate_incorrect_keys_and_values_in_labels():
    labels = {
        'subcid1': {
            'my_key_with_:': 'my_value_:',
        },
    }
    expected_exception = (
        'Each key must match the regular expression `[a-z][-_./\\@0-9a-z]`.\n'
        'But key `my_key_with_:` does not match pattern.\n'
        'Each value must match the regular expression `[-_./\\@0-9a-z]`.\n'
        'But value `my_value_:` does not match pattern.\n'
    )
    with pytest.raises(DbaasClientError) as excinfo:
        validate_labels(labels)
    assert expected_exception in str(excinfo.value)


def test_validate_incorrect_values_in_labels():
    labels = {
        'subcid1': {
            'abc23': ':',
        },
    }
    expected_exception = (
        'Each value must match the regular expression `[-_./\\@0-9a-z]`.\n' 'But value `:` does not match pattern.\n'
    )
    with pytest.raises(DbaasClientError) as excinfo:
        validate_labels(labels)
    assert expected_exception in str(excinfo.value)


def test_validate_incorrect_labels_with_empty_strings():
    labels = {
        'subcid1': {
            'subcid_id': '',
        },
    }
    expected_exception = (
        'The string length in characters for each value must be `1-63`.\n' 'But value `` has length `0`.\n'
    )
    with pytest.raises(DbaasClientError) as excinfo:
        validate_labels(labels)
    assert expected_exception in str(excinfo.value)


def test_validate_incorrect_labels_with_big_keys():
    big_key = 'my_super_puper_hyper_cool_awful_key_is_bigger_than_your_super_puper_hyper_cool_awful_key'
    labels = {
        'subcid1': {
            big_key: 'ololo',
        },
    }
    expected_exception = (
        'The string length in characters for each key must be `1-63`.\n' f'But key `{big_key}` has length `88`.\n'
    )
    with pytest.raises(DbaasClientError) as excinfo:
        validate_labels(labels)
    assert expected_exception in str(excinfo.value)


def test_validate_incorrect_labels_with_big_values():
    big_value = 'my_super_puper_hyper_cool_awful_value_is_bigger_than_your_super_puper_hyper_cool_awful_value'
    labels = {
        'subcid1': {
            'ololo': big_value,
        },
    }
    expected_exception = (
        'The string length in characters for each value must be `1-63`.\n' f'But value `{big_value}` has length `92`.\n'
    )
    with pytest.raises(DbaasClientError) as excinfo:
        validate_labels(labels)
    assert expected_exception in str(excinfo.value)


def test_validate_incorrect_big_labels():
    labels = dict()
    inner_dict = {x: x for x in string.ascii_lowercase}
    inner_dict.update({x * 2: x for x in string.ascii_lowercase})
    inner_dict.update({x * 3: x for x in string.ascii_lowercase})
    labels['subcid1'] = inner_dict
    print(labels)
    expected_exception = 'No more than 64 label\'s pair per resource was allowed.\nBut you have `78` pairs of labels.\n'
    with pytest.raises(DbaasClientError) as excinfo:
        validate_labels(labels)
    assert expected_exception in str(excinfo.value)
