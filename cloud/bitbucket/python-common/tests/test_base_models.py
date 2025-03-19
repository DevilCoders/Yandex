import pytest

from schematics import types as schematics_types
from schematics import exceptions as schematics_exceptions
from yc_common.clients.models import base as base_models


class _TestBaseModel(base_models.BasePublicModel):
    some_field1 = schematics_types.StringType()


class _TestModel1(base_models.BasePublicUpdateModel):
    some_field1 = schematics_types.StringType()
    some_field2 = schematics_types.StringType()


class _TestModel2(base_models.BasePublicUpdateModel, _TestBaseModel):
    some_field2 = schematics_types.StringType()


class _TestModel3(_TestBaseModel, base_models.BasePublicUpdateModel):
    some_field2 = schematics_types.StringType()


class _TestModelOverriddenField(base_models.BasePublicUpdateModel, _TestBaseModel):
    form_fields = schematics_types.StringType(serialized_name="fields")
    snake_field = schematics_types.StringType()


@pytest.mark.parametrize("model_class", (_TestModel1, _TestModel2, _TestModel3))
def test_simple_inheritance_no_fields(model_class):
    model_class({"someField1": "value1", "someField2": "value2"})


@pytest.mark.parametrize("model_class", (_TestModel1, _TestModel2, _TestModel3))
def test_simple_inheritance_all_fields(model_class):
    model_class({"someField1": "value1", "someField2": "value2", "updateMask": "someField1,someField2"}, validate=True)


@pytest.mark.parametrize("model_class", (_TestModel1, _TestModel2, _TestModel3))
def test_simple_inheritance_missing_fields(model_class):
    model_class({"someField1": "value1", "someField2": "value2", "updateMask": "someField1"}, validate=True)


@pytest.mark.parametrize("model_class", (_TestModel1, _TestModel2, _TestModel3))
def test_simple_inheritance_extra_fields(model_class):
    with pytest.raises(schematics_exceptions.DataError):
        model_class({"someField1": "value1", "someField2": "value2", "updateMask": "someField1,someField3"}, validate=True)


def test_filter_by_mask():
    result = base_models.filter_by_mask(
        {"k0": "v0", "k1": "v1", "k2": "v2", "k3": None, "k4": None},
        {"k1", "k2", "k3", "k4"},
        {"k2", "k3"}
    )
    assert result == {"k1": "v1", "k2": "v2", "k3": None}


# one can not name model attribute "fields" that's why we need serialized_name here
# checking that BasePublicModel does not change serialized_name if provided
def test_serialized_name_public_model():
    response = _TestModelOverriddenField.new(form_fields="some_fields", snake_field="snake_value").to_api(public=False)
    assert response == {"fields": "some_fields", "snakeField": "snake_value"}
