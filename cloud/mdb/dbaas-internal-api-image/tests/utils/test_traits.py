"""
Test for types
"""
import pytest
from marshmallow.exceptions import ValidationError

from dbaas_internal_api.utils.traits import InvalidInit, ValidString


@pytest.mark.parametrize(
    ['regexp', 'min', 'max', 'blacklist'],
    [('0+', 1, 1, [])],
)
def test_init_valid(mocker, regexp, min, max, blacklist):
    s = ValidString(regexp=regexp, min=min, max=max, blacklist=blacklist)
    assert s.regexp == regexp
    assert s.min == min
    assert s.max == max
    assert s.blacklist == blacklist


@pytest.mark.parametrize(
    ['regexp', 'min', 'max', 'blacklist'],
    [
        ('0+', -1, 1, []),
        ('0+', 2, 1, []),
    ],
)
def test_init_invalid(mocker, regexp, min, max, blacklist):
    with pytest.raises(InvalidInit):
        ValidString(regexp=regexp, min=min, max=max, blacklist=blacklist)


@pytest.mark.parametrize(
    ['regexp', 'min', 'max', 'blacklist', 'values'],
    [
        ('0+', 1, 3, [], ['0', '00', '000']),
        ('[abc]+', 1, 3, [], ['a', 'aa', 'aaa', 'abc', 'cba', 'bc', 'ca']),
        ('[a-zA-Z0-9_-]+', 1, 63, [], ['db-test']),
    ],
)
def test_valid_string(mocker, regexp, min, max, blacklist, values):
    vs = ValidString(regexp=regexp, min=min, max=max, blacklist=blacklist)

    for value in values:
        vs.validate(value)


@pytest.mark.parametrize(
    ['regexp', 'min', 'max', 'blacklist', 'values'],
    [
        ('0+', 1, 1, [], ['', '00', '1']),
        ('[abc]+', 1, 3, [], ['', 'd', 'abca', 'adb']),
    ],
)
def test_invalid_string(mocker, regexp, min, max, blacklist, values):
    vs = ValidString(regexp=regexp, min=min, max=max, blacklist=blacklist)

    for value in values:
        with pytest.raises(ValidationError):
            vs.validate(value)
