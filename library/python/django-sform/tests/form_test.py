# encoding: utf-8
from __future__ import unicode_literals

import sform

from collections import OrderedDict

import pytest
from unittest.mock import Mock, patch
import operator
from django.core.exceptions import ValidationError as VE

from .fixtures import form_data


def test_mc_new():
    f1 = sform.IntegerField()
    f2 = sform.IntegerField()
    f3 = sform.IntegerField()

    class Form1(sform.SForm):
        not_field = 1
        field_1 = f1
        field_2 = f2

    class Form2(Form1):
        not_field = 2
        field_3 = f3

    assert Form1.not_field == 1
    assert Form2.not_field == 2

    assert f1.creation_counter < f2.creation_counter < f3.creation_counter

    assert Form1.base_fields == OrderedDict([('field_1', f1), ('field_2', f2)])
    assert Form2.base_fields == OrderedDict([('field_1', f1), ('field_2', f2), ('field_3', f3)])


@pytest.mark.parametrize('has_data,has_initial', [
    (False, False),
    (True, False),
    (False, True),
    (True, True),
])
def test_init(has_data, has_initial, form_cls):

    i_data = form_data if has_data else None
    i_initial = form_data if has_initial else None
    r_data = form_data if has_data else {}
    r_initial = form_data if has_initial else {}

    form = form_cls(
        data=i_data,
        initial=i_initial,
    )

    assert form.data == r_data
    assert form.initial == r_initial
    assert form.base_initial == r_initial
    assert form.base_data == r_data
    assert form._errors is None
    assert form.cleaned_data == {}
    assert list(form.fields.keys()) == ['int_field', 'field_set', 'field_set_grid', 'int_grid']
    assert form.fields_state == {
        'int_field': sform.NORMAL,
        'field_set': sform.NORMAL,
        'field_set_grid': sform.NORMAL,
        'int_grid': sform.NORMAL,
    }


class Obj(object):
    f1 = 1


GIV_PARAMS = [
    (operator.attrgetter, Obj()),
    (operator.itemgetter, {'f1': 1}),
]


@pytest.mark.parametrize('getter,obj', GIV_PARAMS)
def test_get_initial_value_w_getter(getter, obj):

    field1 = sform.IntegerField()
    field2 = sform.IntegerField(default=2)

    class Form(sform.SForm):
        default_getter = getter
        f1 = field1
        f2 = field2

    form = Form(initial=obj)
    assert form._get_initial_value(name='f1', field=field1) == 1
    assert form._get_initial_value(name='f2', field=field2) == 2


def test_get_initial_value_field_getter():
    field = sform.IntegerField()
    class Form(sform.SForm):
        f = field
        def get_f(self, initial):
            return initial.get('f')

    form = Form(initial={'f': 1})
    assert form._get_initial_value(name='f', field=field) == 1


def test_errors_as_dict():
    form = sform.SForm()
    form._errors = {
        ('',): [VE('Form Level', code='form_level')],
        ('f',): [VE('Field Level', code='field_level')],
        ('f', '0'): [VE('Item Level', code='item_level')],
        ('f', '0', 'if'): [VE('Item Field Level', code='item_field_level')],
        ('f', 'if'): [VE('Iner Level', code='iner_level')],
        ('le',): [VE('Error 1', code='error_1'), VE('Error 2', code='error_2')],
        ('wp',): [VE('With Params', code='with_params', params={'p': 1})],
    }

    assert form.errors_as_dict() == {'errors': {
        '': [{'code': 'form_level'}],
        'f': [{'code': 'field_level'}],
        'f[0]': [{'code': 'item_level'}],
        'f[0][if]': [{'code': 'item_field_level'}],
        'f[if]': [{'code': 'iner_level'}],
        'le': [{'code': 'error_1'}, {'code': 'error_2'}],
        'wp': [{'code': 'with_params', 'params': {'p': 1}}],
    }}


def test__field_is_readonly():
    old_value = 1

    form = sform.SForm(initial={'f': old_value})

    form.cleaned_data = {'f': 2}
    form._field_is_readonly(sform.NORMAL, 'f', old_value)
    assert form.cleaned_data['f'] == 2

    form.cleaned_data = {'f': 2}
    form._field_is_readonly(sform.READONLY, 'f', old_value)
    assert form.cleaned_data['f'] == old_value

    form.cleaned_data = {}
    form.initial = {}
    form._field_is_readonly(sform.READONLY, 'f', old_value)
    assert form.cleaned_data == {}


def test__check_field_custom_clean():
    form = sform.SForm()
    form.cleaned_data = {'f': 1}
    form._check_field_custom_clean('f', 2)
    assert form.cleaned_data == {'f': 1}

    form.clean_f = Mock(return_value=2)
    form.cleaned_data = {'f': 1}
    form._check_field_custom_clean('f', 2)
    assert form.cleaned_data == {'f': 2}
    form.clean_f.assert_called_once_with(2)


def test__handle_field_exception():
    form = sform.SForm()

    errors = {'e': 'e'}
    e = VE('e', code='e')

    m = Mock(return_value=errors)
    with patch('sform._extract_errors', side_effect=m):
        form._errors = {}
        form.cleaned_data = {'f': 1}
        form._handle_field_exception('f', e)
        m.assert_called_once_with(name='f', e=e)
        assert form.cleaned_data == {}
        assert form._errors == errors

    m = Mock(return_value=errors)
    with patch('sform._extract_errors', side_effect=m):
        form._errors = {}
        form.cleaned_data = {'a': 1}
        form._handle_field_exception('f', e)
        m.assert_called_once_with(name='f', e=e)
        assert form.cleaned_data == {'a': 1}
        assert form._errors == errors


def test_gridfield_initials():

    class SubForm(sform.SForm):
        key = sform.IntegerField(state=sform.READONLY)
        value = sform.CharField(state=sform.NORMAL)

    def custom_values_matcher(old_values, new_values, default):
        old_values = {old['key']: old for old in old_values}
        for new in new_values:
            old = old_values.get(new['key'], default)
            yield old, new

    class DefaultMatcherForm(sform.SForm):
        field = sform.GridField(sform.FieldsetField(SubForm))

    class CustomMatcherForm(sform.SForm):
        field = sform.GridField(
            sform.FieldsetField(SubForm),
            values_matcher=custom_values_matcher,
        )

    obj1 = {'key': 1, 'value': 'a'}
    obj2 = {'key': 2, 'value': 'b'}

    initial = {'field': [obj1, obj2]}
    data = {'field': [obj2]}

    form = DefaultMatcherForm(initial=initial, data=data)
    assert form.is_valid()
    cleaned = form.cleaned_data['field'][0]
    assert cleaned != obj2
    assert cleaned['key'] == obj1['key']
    assert cleaned['value'] == obj2['value']

    form = CustomMatcherForm(initial=initial, data=data)
    assert form.is_valid()
    assert form.cleaned_data['field'][0] == obj2
