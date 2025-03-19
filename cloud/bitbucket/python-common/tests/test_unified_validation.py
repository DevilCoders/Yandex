"""Test yc_common.unified_validation_module."""

import sys
import pytest

from schematics.exceptions import ValidationError

from yc_common import fields, models
from yc_common.unified_validation import validate_untrusted_data


EPSILON = sys.float_info.epsilon


def test_unified_validation_primitive():
    assert validate_untrusted_data(models.StringType(), "The string.") == "The string."
    assert validate_untrusted_data(models.IntType(), 42) == 42
    assert (validate_untrusted_data(models.FloatType(), 9.1) - 9.1) <= EPSILON
    assert validate_untrusted_data(models.BooleanType(), True)
    assert not validate_untrusted_data(models.BooleanType(), False)
    assert validate_untrusted_data(models.ListType(models.IntType), [1, 2, 3]) == [1, 2, 3]
    assert validate_untrusted_data(models.DictType(field=models.IntType()), {"a": 1, "b": 2}) == {"a": 1, "b": 2}
    assert validate_untrusted_data(models.SchemalessDictType(), {"a": 1, "b": 2}) == {"a": 1, "b": 2}


def test_negative_unified_validation_primitive():
    with pytest.raises(ValidationError):
        validate_untrusted_data(models.StringType(max_length=1), "The string.")

    with pytest.raises(ValidationError):
        validate_untrusted_data(models.IntType(min_value=10), 9)

    with pytest.raises(ValidationError):
        validate_untrusted_data(models.IntType(min_value=10), "NAN")

    with pytest.raises(ValidationError):
        validate_untrusted_data(models.FloatType(min_value=10), "Not a number.")

    with pytest.raises(ValidationError):
        validate_untrusted_data(models.BooleanType(), "The False.")

    with pytest.raises(ValidationError):
        validate_untrusted_data(models.ListType(models.IntType), [1, "Two", 3])

    with pytest.raises(ValidationError):
        validate_untrusted_data(models.DictType(field=models.IntType()), {"a": "nan", "b": "nan"})

    with pytest.raises(ValidationError):
        validate_untrusted_data(models.DictType(field=models.IntType()), [1, 2, 3])

    with pytest.raises(ValidationError):
        validate_untrusted_data(models.SchemalessDictType(), [1, 2, 3])


def test_unified_validation_model():
    class _Struct(models.Model):
        struct_field = fields.BooleanType(required=True)

    class _TestModel(models.Model):
        required_field = fields.StringType(required=True, min_length=5)
        optional_field = fields.ModelType(_Struct)

    model = validate_untrusted_data(models.ModelType(_TestModel), {
        "required_field": "Five.",
    })
    assert model.required_field == "Five."
    assert model.optional_field is None

    model = validate_untrusted_data(models.ModelType(_TestModel), {
        "required_field": "Five!",
        "optional_field": {
            "struct_field": True
        }
    })
    assert model.required_field == "Five!"
    assert model.optional_field.struct_field


def test_unified_validation_model():
    class _Struct(models.Model):
        struct_field = fields.BooleanType(required=True)

    class _TestModel(models.Model):
        required_field = fields.StringType(required=True, min_length=5)
        optional_field = fields.ModelType(_Struct)

    with pytest.raises(ValidationError):
        validate_untrusted_data(models.ModelType(_TestModel), {})

    with pytest.raises(ValidationError):
        validate_untrusted_data(models.ModelType(_TestModel), {
            "optional_field": {}
        })

    with pytest.raises(ValidationError):
        validate_untrusted_data(models.ModelType(_TestModel), {
            "optional_field": "This is not object."
        })

    with pytest.raises(ValidationError):
        validate_untrusted_data(models.ModelType(_TestModel), {
            "extra": 1
        })


def test_unified_validation_non_strict_model():
    class _TestModel(models.Model):
        required_field = fields.IntType(required=True)

    model = validate_untrusted_data(models.ModelType(_TestModel), {
        "required_field": 1,
        "extra_field": "Does not matter.",
    }, partial=False, strict=False)
    assert model.required_field == 1
