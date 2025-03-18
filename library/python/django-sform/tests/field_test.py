# encoding: utf-8
from __future__ import unicode_literals

import pytest
from unittest.mock import Mock

import sform


@pytest.mark.parametrize('state,default,trim_on_empty,kwargs', [
    (sform.READONLY, None, True, {'a': 1}),
    (sform.REQUIRED, '', False, {'a': 1, 'b': 2}),
    (sform.NORMAL, 1, False, {'a': 1}),
    (None, '2', False, {}),
])
def test_init(state, default, trim_on_empty, kwargs):
    creation_counter = sform.Field.creation_counter

    field = sform.Field(state=state, default=default, trim_on_empty=trim_on_empty, **kwargs)

    assert sform.Field.creation_counter == creation_counter + 1
    assert field.state == state or sform.NORMAL
    assert field.default == default
    assert field.trim_on_empty == trim_on_empty
    assert field.kwargs == kwargs


def test_bool_fields():
    assert sform.BooleanField().extract_new_value({}, 'a') is False
    assert sform.NullBooleanField().extract_new_value({}, 'a') is None


bool_test_values = [
    ('true', True),
    ('True', True),
    ('1', True),
    (True, True),

    ('false', False),
    ('False', False),
    ('0', False),
    (False, False),

    ('null', None),
    ('Null', None),
    ('something', None),
    ('', None),
    (None, None),
]


@pytest.mark.parametrize('raw,result', bool_test_values)
def test_validated_boolean_field(raw, result):
    assert sform.ValidatedNullBooleanField().to_python(raw) is result


def test_init_w_required_param():
    with pytest.raises(AssertionError):
        sform.Field(required=True)


def test_type_name():
    assert sform.Field().type_name == 'field'

    class TestField(sform.Field):
        pass
    assert TestField().type_name == 'test'

    class Foo(sform.Field):
        pass
    assert Foo().type_name == 'foo'

    class Bar(sform.Field):
        type_name = 'foo'

    assert Bar().type_name == 'foo'


def test_structure_as_dict():

    class TestField(sform.Field):
        type_name = 'foo'
        _get_base_field_dict = Mock(return_value={})

    field = TestField(default=1)
    field_dict = field.structure_as_dict('pf', 'f', sform.REQUIRED, {}, {})

    assert field_dict == {'value': 1, 'type': 'foo', 'key': 'f'}

    field._get_base_field_dict.assert_called_once_with('pf', 'f', sform.REQUIRED)


def test_data_as_dict():

    class TestField(sform.Field):
        type_name = 'foo'
        _get_base_field_dict = Mock(return_value={})

    field = TestField(default=1)
    field_dict = field.data_as_dict('pf', 'f', 2, sform.REQUIRED, {}, {})

    assert field_dict == {'value': 2}

    field._get_base_field_dict.assert_called_once_with('pf', 'f', sform.REQUIRED)


def test__get_base_field_dict():

    field_dict = sform.Field()._get_base_field_dict('pf', 'f', sform.REQUIRED)
    assert field_dict == {'name': 'pf[f]', 'required': True}

    field_dict = sform.Field()._get_base_field_dict('', 'f', sform.NORMAL)
    assert field_dict == {'name': 'f'}

    field_dict = sform.Field()._get_base_field_dict('', 'f', sform.READONLY)
    assert field_dict == {'name': 'f', 'readonly': True}
