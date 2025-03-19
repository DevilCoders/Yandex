"""Tests formatting tools"""

import pytest

from yc_common import constants
from yc_common.formatting import KeepFormat, parse_human_size, underscore_to_lowercamelcase, camelcase_to_underscore, hyphen_to_underscore


def test_parse_human_size():
    with pytest.raises(ValueError) as e:
        parse_human_size("1A")
    assert str(e.value) == "Invalid size"

    for unit in ("", "K", "M", "G", "T"):
        value = "-1" + unit
        with pytest.raises(ValueError) as e:
            parse_human_size(value)
        assert str(e.value) == "Invalid size"

        value = "0" + unit
        assert parse_human_size(value) == 0
        with pytest.raises(ValueError) as e:
            parse_human_size(value, allow_zero=False)
        assert str(e.value) == "Invalid size"

    assert parse_human_size("1") == 1
    assert parse_human_size("2K") == 2 * constants.KILOBYTE
    assert parse_human_size("30M") == 30 * constants.MEGABYTE
    assert parse_human_size("401G") == 401 * constants.GIGABYTE
    assert parse_human_size("5000T") == 5000 * constants.TERABYTE


def test_underscore_to_lowercamelcase():
    original = {
        "key1": {"sub_key1": 1},
        2: {"sub_key2": [3, 4]},
        "some_key3": "some_value",
        "keep_key": KeepFormat({"sub_keep": 1}),
    }
    expected = {
        "key1": {"subKey1": 1},
        2: {"subKey2": [3, 4]},
        "someKey3": "some_value",
        "keepKey": {"sub_keep": 1},
    }

    assert underscore_to_lowercamelcase(original) == expected


def test_camelcase_to_underscore():
    original = {
        "key1": {"subKey1": 1},
        2: {"subKey2": [3, 4]},
        "someKey3": "someValue",
        5: [{"subKey4": 6}],
        "keepKey": KeepFormat({"subKeep": 1}),
    }
    expected = {
        "key1": {"sub_key1": 1},
        2: {"sub_key2": [3, 4]},
        "some_key3": "someValue",
        5: [{"sub_key4": 6}],
        "keep_key": {"subKeep": 1},
    }

    assert camelcase_to_underscore(original) == expected


def test_hyphen_to_underscore():
    original = {
        "key1": {"sub-key1": 1},
        2: {"sub-key2": [3, 4]},
        "some-key3": "some-value",
        "some_key4": "some_value",
        "keep-key": KeepFormat({"sub-keep": 1}),
    }
    expected = {
        "KEY1": {"SUB_KEY1": 1},
        2: {"SUB_KEY2": [3, 4]},
        "SOME_KEY3": "some-value",
        "SOME_KEY4": "some_value",
        "KEEP_KEY": {"sub-keep": 1},
    }
    assert hyphen_to_underscore(original) == expected
