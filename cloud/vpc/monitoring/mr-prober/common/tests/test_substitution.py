from typing import Any, Dict

import pytest

from common.substitution import (
    TYPES_ALLOWED_FOR_PARTIAL_SUBSTITUTION,
    substitute,
    get_value_of_composite_key,
    ErrorEmptyKey,
    SubstituteKeyError,
)

testdata_empty_variables = {}
testdata_variables = {
    "key1": {},
    "key2": {
        "sub_key2": 3,
        "sub_key_int": 1,
        "sub_key_str": "1",
        "sub_key_list": ["1", "2"],
        "": {},
        "2": 2,
    },
    "key3": "value3",
}


@pytest.mark.parametrize(
    "value,expected,variables",
    [
        ["1", "1", testdata_empty_variables],
        ["{1}", "{1}", testdata_empty_variables],
        ["${key1}", {}, testdata_variables],
        ["${key2.sub_key_int}", 1, testdata_variables],
        ["${key2.sub_key_list}", ["1", "2"], testdata_variables],
        ["${key2.sub_key_int}2", "12", testdata_variables],
        ["${key${key2.sub_key2}}", "value3", testdata_variables],
        ["${{passed_variables}}", "${passed_variables}", testdata_variables],
        ["${{key${key2.sub_key2}}}", "${key3}", testdata_variables],
        ["${key${{key2}}}", "${key${key2}}", testdata_variables],
        ["${ key1 }", {}, testdata_variables],
        ["${ key2.sub_key_int }", 1, testdata_variables],
        ["${key2.sub_key2}.${key2.sub_key_int}.${key3}", "3.1.value3", testdata_variables],
        ["${key2.sub_key2}.${key2.sub_key_int}.${key3}.${key3}", "3.1.value3.value3", testdata_variables],
    ]
)
def test_substitute_positive_cases(value: Any, expected: Any, variables: Dict[str, Any]):
    result = substitute(value, variables)
    assert result == expected


def test_substitute_negative_cases():
    with pytest.raises(SubstituteKeyError) as ex:
        substitute("${not_exists_key}", testdata_variables)
    assert ex.value.key == "not_exists_key"
    assert ex.value.value == "${not_exists_key}"
    assert ex.value.depth == 0

    with pytest.raises(SubstituteKeyError) as ex:
        substitute("${key${key${key2.sub_key_int}.${not_exists_key}}}", testdata_variables)
    assert ex.value.key == "not_exists_key"
    assert ex.value.value == "${key${key1.${not_exists_key}}}"
    assert ex.value.depth == 1

    with pytest.raises(TypeError) as ex:
        substitute("${key2.sub_key_list}_", testdata_variables)
    assert f"when substituting part of a string, only the following types can be used: " \
           f"{', '.join(map(lambda x: x.__name__, TYPES_ALLOWED_FOR_PARTIAL_SUBSTITUTION))}" in str(ex.value)

    with pytest.raises(ErrorEmptyKey) as ex:
        substitute("${key2.}", testdata_variables)
    assert "empty key after position 4 in value 'key2.'" in str(ex.value)

    with pytest.raises(Exception) as ex:
        substitute("${key${key${key${key${key${key${key${key${key${key${key${key${key2.2}.2}.2}.2}.2}.2}.2}.2}.2}.2}.2}.2}}", testdata_variables)
    assert "the limit of the variables count has been reached" in str(ex.value)


@pytest.mark.parametrize(
    "composite_key,expected,variables_map",
    [
        ["key1", {}, testdata_variables],
        ["key2.sub_key_int", 1, testdata_variables],
    ]
)
def test_get_value_of_composite_keys_positive_cases(composite_key: str, expected: Any, variables_map: Dict[str, Any]):
    assert get_value_of_composite_key(composite_key, variables_map) == expected


def test_get_value_of_composite_keys_negative_cases():
    with pytest.raises(KeyError) as ex:
        assert get_value_of_composite_key("not_exists_key", testdata_variables) is None
    assert "not_exists_key" in str(ex.value)

    with pytest.raises(KeyError) as ex:
        assert get_value_of_composite_key("key1.not_exists_sub_key", testdata_variables) is None
    assert "key1.not_exists_sub_key" in str(ex.value)

    with pytest.raises(ValueError) as ex:
        assert get_value_of_composite_key("key2.sub_key_str.sub_sub_key1", testdata_variables) is None
    assert "key 'key2.sub_key_str.sub_sub_key1' assumes than value of 'key2.sub_key_str' must be 'dict', " \
           "but found 'str'" in str(ex.value)

    with pytest.raises(ErrorEmptyKey) as ex:
        assert get_value_of_composite_key("key2.", testdata_variables) == "value"
    assert "empty key after position 4 in value 'key2.'" in str(ex.value)
