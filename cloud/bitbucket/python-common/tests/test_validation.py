"""Test validation"""

import pytest
import schematics.models as schematics_models
import schematics.exceptions as schematics_exceptions

from yc_common.validation import RequestValidationError, \
    validate_resource_name, ZoneIdType, ResourceDescriptionType


def test_description_simple():
    text = "test тест gepäck"
    assert ResourceDescriptionType().to_native(text) == text


def test_description_emoji():
    text = "some 👍 emoji"
    assert ResourceDescriptionType().to_native(text) == text


def test_description_strip_whitespaces():
    assert ResourceDescriptionType().to_native("test  \t  \nтест") == "test тест"


@pytest.mark.parametrize("text", ("", "\t"))
def test_description_empty(text):
    assert ResourceDescriptionType().to_native(text) == ""


def test_description_strip_control_characters():
    text = "".join(chr(c) for c in range(128))
    expected = r"""!"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~"""
    assert ResourceDescriptionType().to_native(text) == expected


def test_resource_name_validation_success():
    validate_resource_name("correct-name1")


def test_resource_name_validation_fail_small():
    with pytest.raises(RequestValidationError):
        validate_resource_name("")


def test_resource_name_validation_fail_large():
    with pytest.raises(RequestValidationError):
        validate_resource_name("a" * 64)


def test_resource_name_validation_minus_at_end():
    with pytest.raises(RequestValidationError):
        validate_resource_name("aaa-")


def test_zone_id_validation_success():
    class M(schematics_models.Model):
        z = ZoneIdType()

    M({"z": "ru-central1-a"}, validate=True)


def test_zone_id_validation_fail_bad_symbol():
    class M(schematics_models.Model):
        z = ZoneIdType()

    with pytest.raises(schematics_exceptions.DataError):
        M({"z": "ru-central1.a"}, validate=True)


def test_zone_id_validation_fail_length():
    class M(schematics_models.Model):
        z = ZoneIdType()

    with pytest.raises(schematics_exceptions.DataError):
        M({"z": "a" * 300}, validate=True)
