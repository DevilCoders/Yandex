"""Unit tests for yc_common.fields module."""

import datetime
import decimal

from functools import partial
from uuid import uuid4

import pytest
from schematics import exceptions

from yc_common import models
from yc_common import fields


_TARGET_TYPE = {
    fields.BooleanType: models.BooleanType,
    fields.DateTimeType: models.DateTimeType,
    fields.DateType: models.DateType,
    fields.DecimalType: models.DecimalType,
    fields.DictType: models.DictType,
    fields.FloatType: models.FloatType,
    fields.IntType: models.IntType,
    fields.JsonDictType: models.JsonDictType,
    fields.JsonListType: models.JsonListType,
    fields.JsonModelType: models.JsonModelType,
    fields.JsonSchemalessDictType: models.JsonSchemalessDictType,
    fields.LongType: models.LongType,
    fields.ListType: models.ListType,
    fields.ModelType: models.ModelType,
    fields.SchemalessDictType: models.SchemalessDictType,
    fields.StringType: models.StringType,
    fields.UUIDType: models.UUIDType,
}


class _EmptyModel(models.Model):
    pass


@pytest.mark.parametrize("required", (True, False,))
def test_required_and_target_type(required: bool):
    _test_required_and_target_type(fields.BooleanType, fields.BooleanType, required)
    _test_required_and_target_type(fields.DateTimeType, fields.DateTimeType, required)
    _test_required_and_target_type(fields.DateType, fields.DateType, required)
    _test_required_and_target_type(fields.DecimalType, fields.DecimalType, required)
    _test_required_and_target_type(fields.DictType, partial(fields.DictType, fields.IntType(), required=required), required)
    _test_required_and_target_type(fields.FloatType, fields.FloatType, required)
    _test_required_and_target_type(fields.IntType, fields.IntType, required)
    _test_required_and_target_type(fields.JsonDictType, partial(fields.JsonDictType, fields.IntType(), required=required), required)
    _test_required_and_target_type(fields.JsonListType, partial(fields.JsonListType, fields.IntType(), required=required), required)
    _test_required_and_target_type(fields.JsonModelType, partial(fields.JsonModelType, _EmptyModel, required=required), required)
    _test_required_and_target_type(fields.JsonSchemalessDictType, fields.JsonSchemalessDictType, required)
    _test_required_and_target_type(fields.LongType, fields.LongType, required)
    _test_required_and_target_type(fields.ListType, partial(fields.ListType, fields.IntType(), required=required), required)
    _test_required_and_target_type(fields.ModelType, partial(fields.ModelType, _EmptyModel, required=required), required)
    _test_required_and_target_type(fields.SchemalessDictType, fields.SchemalessDictType, required)
    _test_required_and_target_type(fields.StringType, fields.StringType, required)
    _test_required_and_target_type(fields.UUIDType, fields.UUIDType, required)


def _test_required_and_target_type(field_func, field_constructor, required: bool):
    field_type = field_constructor(required=required)
    target_type = _TARGET_TYPE[field_func]
    # I don't know why PyCharm thinks target_type is "parametrized generic". Suppressed...
    # noinspection PyTypeHints
    assert isinstance(field_type, target_type)

    class TestModel(models.Model):
        field = field_type

    model = TestModel()

    if required:
        with pytest.raises(exceptions.DataError):
            model.validate()
    else:
        model.validate()
        assert model.field is None


@pytest.mark.parametrize("use_default", (True, False,))
def test_value(use_default: bool):
    _test = partial(_test_value, use_default)

    _test(fields.BooleanType, True)
    _test(fields.BooleanType, False)
    _test(fields.DateTimeType, "2017-01-01 01:01:01", datetime.datetime(2017, 1, 1, 1, 1, 1), serialized_format="%Y-%m-%d %H:%M:S")
    _test(fields.DateType, "01 01 2017", datetime.date(2017, 1, 1), formats=["%Y-%m-%d", "%d %m %Y"])
    _test(fields.DecimalType, 42, decimal.Decimal(42))
    _test(partial(fields.DictType, fields.IntType()), {"foo": 1})
    _test(partial(fields.DictType, fields.StringType(), key=fields.IntType()), {1: "foo"})
    _test(fields.FloatType, 42.2)
    _test(fields.IntType, 42)
    _test(partial(fields.JsonDictType, fields.IntType()), {"foo": 1})
    _test(partial(fields.JsonDictType, fields.StringType(), key=fields.IntType()), {1: "foo"})
    _test(partial(fields.JsonListType, fields.StringType()), ["a", "b", "c"])
    _test(partial(fields.JsonModelType, _EmptyModel), _EmptyModel())
    _test(fields.JsonSchemalessDictType, {"foo": 1})
    _test(fields.StringType, "Have a nice day")
    _test(fields.StringType, "Have a nice day", choices=["Have a nice day"])
    u4 = uuid4()
    _test(fields.UUIDType, str(u4), u4)
    _test(fields.UUIDType, u4, u4)


def _test_value(use_default: bool, field_constructor, value, required_value=None, **kwargs):
    if use_default:
        class TestModel(models.Model):
            field = field_constructor(default=value, required=True, **kwargs)
        model = TestModel()
    else:
        class TestModel(models.Model):
            field = field_constructor(required=True, **kwargs)
        model = TestModel()
        model.field = value
    model.validate()
    assert model.field == value if required_value is None else required_value


def test_bad_values():
    _test_bad_value(fields.DateTimeType, "01:01:01 2017-01-01", formats=["%Y-%m-%d %H:%M:%S"], serialized_format=["%H:%M:%S %Y-%m-%d"])
    _test_bad_value(fields.DateType, "2017-01-01", formats=["%d.%m.%Y"])
    _test_bad_value(fields.IntType, 42, min_value=0, max_value=41)
    _test_bad_value(fields.IntType, 42, min_value=43, max_value=100)
    _test_bad_value(fields.LongType, 42, min_value=0, max_value=41)
    _test_bad_value(fields.LongType, 42, min_value=43, max_value=100)
    _test_bad_value(partial(fields.JsonListType, fields.IntType()), [1, 2, 3], min_size=1, max_size=2)
    _test_bad_value(partial(fields.JsonListType, fields.IntType()), [1, 2, 3], min_size=4, max_size=10)
    _test_bad_value(partial(fields.ListType, fields.IntType()), [1, 2, 3], min_size=1, max_size=2)
    _test_bad_value(partial(fields.ListType, fields.IntType()), [1, 2, 3], min_size=4, max_size=10)
    _test_bad_value(partial(fields.JsonDictType, fields.IntType(), key=fields.IntType()), {"a": 1})
    _test_bad_value(partial(fields.JsonDictType, fields.IntType(), key=fields.IntType()), {1: "a"})
    _test_bad_value(partial(fields.DictType, fields.IntType(), key=fields.IntType()), {"a": 1})
    _test_bad_value(partial(fields.DictType, fields.IntType(), key=fields.IntType()), {1: "a"})
    _test_bad_value(fields.StringType, "Have a nice day", choices=["Don't have a nice day"])
    _test_bad_value(fields.StringType, "Have a nice day", regex="Lol")
    _test_bad_value(fields.UUIDType, "Like an UUID")


def _test_bad_value(field_constructor, bad_value, **kwargs):
    class TestModel(models.Model):
        field = field_constructor(required=True, **kwargs)
    model = TestModel()
    model.field = bad_value
    with pytest.raises(exceptions.DataError):
        model.validate()
