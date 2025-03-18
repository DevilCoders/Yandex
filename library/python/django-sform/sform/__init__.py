# coding: utf-8
from __future__ import unicode_literals
import copy
import six

import operator

from collections import OrderedDict

from django import forms
from django.core.exceptions import ValidationError, FieldDoesNotExist
from django.utils.translation import get_language
from django.utils.encoding import smart_text


__all__ = (
    'SForm', 'FieldsetField', 'GridField',
    'IntegerField', 'CharField', 'BooleanField', 'NullBooleanField',
    'DateField', 'DateTimeField', 'TimeField', 'EmailField',
    'ChoiceField', 'MultipleChoiceField',
    'ModelChoiceField', 'ModelMultipleChoiceField',
    'DecimalField',
    'SuggestField', 'MultipleSuggestField',
)

# Django fields
# 'IntegerField','TimeField','DateTimeField', 'TimeField','RegexField',
# 'FileField', 'ImageField', 'URLField','NullBooleanField',
# 'MultipleChoiceField',
# 'ComboField', 'MultiValueField', 'FloatField', 'DecimalField',
# 'SplitDateTimeField', 'IPAddressField', 'GenericIPAddressField',
# 'FilePathField',
# 'SlugField', 'TypedChoiceField', 'TypedMultipleChoiceField'


REQUIRED = 'required'
NORMAL = 'normal'
READONLY = 'readonly'


class Field(object):

    creation_counter = 0

    def __init__(self, state=NORMAL, default='', trim_on_empty=False, **kwargs):
        assert 'required' not in kwargs, '"sform.Field" does not support "required" param. Use "state".'

        self.creation_counter = Field.creation_counter
        Field.creation_counter += 1

        self.state = state
        self.kwargs = kwargs
        self.trim_on_empty = trim_on_empty
        self.default = default

    def extract_new_value(self, data, name):
        return data.get(name, self.default)

    def clean(self, new_value, old_value, required, trim_on_empty, base_initial, base_data):
        dj_field = self.get_dj_field(required=required)
        return dj_field.clean(new_value)

    def get_dj_field(self, required=False):
        raise NotImplementedError()

    def _get_base_field_dict(self, prefix, name, state):
        field_dict = {
            'name': '%s[%s]' % (prefix, name) if prefix else name,
        }
        readonly = state == READONLY
        if readonly:
            field_dict['readonly'] = readonly
        required = state == REQUIRED
        if required:
            field_dict['required'] = required
        return field_dict

    @property
    def type_name(self):
        class_name = self.__class__.__name__.lower()
        if class_name == 'field':
            return 'field'
        elif class_name.endswith('field'):
            return class_name[:-5]
        else:
            return class_name

    def data_as_dict(self, prefix, name, value, state, base_initial, base_data):
        field_dict = self._get_base_field_dict(prefix, name, state)
        field_dict['value'] = value
        return field_dict

    def structure_as_dict(self, prefix, name, state, base_initial, base_data):
        field_dict = self._get_base_field_dict(prefix, name, state)

        field_dict.update({
            'value': self.default,
            'type': self.type_name,
            'key': name,
        })
        # TODO унести на уровень соответствующих полей.
        if 'max_length' in self.kwargs:
            field_dict['maxlength'] = self.kwargs['max_length']
        # -----

        return field_dict


class BooleanField(Field):
    def __init__(self, *args, **kwargs):
        kwargs['default'] = kwargs.get('default', False)
        super(BooleanField, self).__init__(*args, **kwargs)

    def get_dj_field(self, required=False):
        return forms.BooleanField(required=False, **self.kwargs)


class ValidatedNullBooleanField(forms.NullBooleanField):
    def validate(self, value):
        if value is None and self.required:
            raise ValidationError(
                self.error_messages['required'],
                code='required',
            )

    def to_python(self, value):
        if isinstance(value, six.string_types):
            value = value.capitalize()
        return super(ValidatedNullBooleanField, self).to_python(value)


class NullBooleanField(BooleanField):
    def __init__(self, *args, **kwargs):
        kwargs['default'] = kwargs.get('default')
        super(NullBooleanField, self).__init__(*args, **kwargs)

    def get_dj_field(self, required=False):
        return ValidatedNullBooleanField(required=required, **self.kwargs)


class IntegerField(Field):
    def get_dj_field(self, required=False):
        return forms.IntegerField(required=required, **self.kwargs)


class DateField(Field):
    def get_dj_field(self, required=False):
        return forms.DateField(required=required, **self.kwargs)


class DateTimeField(Field):
    def get_dj_field(self, required=False):
        return forms.DateTimeField(required=required, **self.kwargs)


class TimeField(Field):
    def get_dj_field(self, required=False):
        return forms.TimeField(required=required, **self.kwargs)


class DecimalField(Field):
    def get_dj_field(self, required=False):
        return forms.DecimalField(required=required, **self.kwargs)


class CharField(Field):
    def get_dj_field(self, required=False):
        return forms.CharField(required=required, **self.kwargs)


class EmailField(Field):
    def get_dj_field(self, required=False):
        return forms.EmailField(required=required, **self.kwargs)


class UUIDField(Field):
    def get_dj_field(self, required=False):
        return forms.UUIDField(required=required, **self.kwargs)


class ChoiceField(Field):
    def __init__(self, choices, empty_label=None, *args, **kwargs):
        self.choices = choices
        self.empty_label = empty_label
        super(ChoiceField, self).__init__(*args, **kwargs)

    def get_dj_field(self, required=False):
        return forms.ChoiceField(
            required=required,
            choices=self.choices,
            **self.kwargs
        )

    def structure_as_dict(self, *args, **kwargs):
        field_dict = super(ChoiceField, self).structure_as_dict(*args, **kwargs)

        def choice_gen():
            if self.empty_label is not None:
                yield ('', self.empty_label)
            for value, label in self.choices:
                yield (value, label)

        field_dict['choices'] = [
            {'value': v, 'label': n} for v, n in choice_gen()
        ]

        return field_dict


class MultipleChoiceField(ChoiceField):
    def __init__(self, *args, **kwargs):
        kwargs['default'] = kwargs.get('default', [])
        super(MultipleChoiceField, self).__init__(*args, **kwargs)

    def get_dj_field(self, required=False):
        return forms.MultipleChoiceField(
            required=required,
            choices=self.choices,
            **self.kwargs
        )


class ModelChoiceField(Field):
    def __init__(self, queryset, label_extractor, empty_label=None, *args, **kwargs):
        self.queryset = queryset
        self.label_extractor = label_extractor
        self.empty_label = empty_label
        if not callable(label_extractor):
            def extractor(obj):
                is_en = get_language() != 'ru'
                if is_en:
                    en_name = label_extractor + '_en'
                    if hasattr(obj, en_name):
                        return getattr(obj, en_name)
                return getattr(obj, label_extractor)
            self.label_extractor = extractor
        super(ModelChoiceField, self).__init__(*args, **kwargs)

    def get_queryset(self):
        return self.queryset

    def get_queryset_for_structure(self):
        return self.get_queryset()

    def get_dj_field(self, required=False):
        return forms.ModelChoiceField(
            queryset=self.get_queryset(),
            required=required,
            **self.kwargs
        )

    def structure_as_dict(self, *args, **kwargs):
        field_dict = super(ModelChoiceField, self).structure_as_dict(*args, **kwargs)

        initial = kwargs['base_initial'].get(kwargs['name'])
        if kwargs['state'] == REQUIRED and initial is not None:
            self.empty_label = None

        to_field_name = self.kwargs.get('to_field_name', 'pk')

        def choice_gen():
            if self.empty_label is not None:
                yield ('', self.empty_label)
            for obj in self.get_queryset_for_structure().all():
                yield (getattr(obj, to_field_name), self.label_extractor(obj))

        field_dict['choices'] = [
            {'value': v, 'label': n} for v, n in choice_gen()
        ]

        return field_dict


class ModelMultipleChoiceField(ModelChoiceField):
    def get_dj_field(self, required=False):
        return forms.ModelMultipleChoiceField(
            queryset=self.get_queryset(),
            required=required,
            **self.kwargs
        )


class AbstractSuggestField(Field):
    def __init__(self, queryset, label_fields, *args, **kwargs):
        self.queryset = queryset
        self.label_fields = label_fields
        self.to_field_name = kwargs.get('to_field_name', 'pk')
        super(AbstractSuggestField, self).__init__(*args, **kwargs)

    def get_queryset(self):
        return self.queryset

    def get_queryset_for_structure(self):
        return self.get_queryset()

    def structure_as_dict(self, *args, **kwargs):
        field_dict = super(AbstractSuggestField, self).structure_as_dict(*args, **kwargs)
        field_dict['types'] = [self.get_queryset_for_structure().model.__name__.lower()]
        return field_dict

    def get_label_fields(self, label_fields=None):
        label_fields = label_fields or self.label_fields
        model = self.get_queryset().model
        is_en = get_language() != 'ru'
        fields = (
            label_fields
            if isinstance(label_fields, (list, tuple))
            else [label_fields]
        )

        label_fields = []
        for field in fields:
            if is_en:
                try:
                    model._meta.get_field(field + '_en')
                except FieldDoesNotExist:
                    label_fields.append(field)
                else:
                    label_fields.append(field + '_en')
            else:
                label_fields.append(field)
        return label_fields


class SuggestField(AbstractSuggestField):
    def get_dj_field(self, required=False):
        return forms.ModelChoiceField(
            queryset=self.get_queryset(),
            required=required,
            **self.kwargs
        )

    def data_as_dict(self, *args, **kwargs):
        field_dict = super(SuggestField, self).data_as_dict(*args, **kwargs)

        if field_dict['value'] in (None, ''):
            field_dict['caption'] = ''
        else:
            field_dict['caption'] = ' '.join(
                smart_text(f) for f in (
                    self.get_queryset().model.objects
                    .values_list(*self.get_label_fields())
                    .get(**{self.to_field_name: field_dict['value']})
                )
            )
        return field_dict


class MultipleSuggestField(AbstractSuggestField):
    def __init__(self, *args, **kwargs):
        kwargs['default'] = kwargs.get('default', [])
        super(MultipleSuggestField, self).__init__(*args, **kwargs)

    def get_dj_field(self, required=False):
        return forms.ModelMultipleChoiceField(
            queryset=self.get_queryset(),
            required=required,
            **self.kwargs
        )

    def data_as_dict(self, *args, **kwargs):
        field_dict = super(MultipleSuggestField, self).data_as_dict(*args, **kwargs)

        label_fields = self.get_label_fields(self.label_fields['caption'])
        extra_fields = self.label_fields.get('extra_fields', [])
        all_fields = set(label_fields + extra_fields) | {self.to_field_name}

        field_dict['label_fields'] = self._get_label_fields_data(
            filter_value=field_dict['value'], values_fields=all_fields
        )
        fields_for_delete = all_fields - set(extra_fields)
        for obj_data in field_dict['label_fields'].values():
            self._set_caption(obj_data, label_fields)
            for field in fields_for_delete:
                del obj_data[field]

        return field_dict

    @staticmethod
    def _set_caption(obj_data, label_fields):
        obj_data['caption'] = ' '.join(smart_text(obj_data[field]) for field in label_fields)

    def _get_label_fields_data(self, filter_value, values_fields):
        key = '%s__in' % self.to_field_name
        return {
            row[self.to_field_name]: dict(row)
            for row in (
                self.queryset
                .values(*values_fields)
                .filter(**{key: filter_value})
            )
        }


class FieldsetField(Field):
    def __init__(self, sform_cls, *args, **kwargs):
        self.sform_cls = sform_cls
        kwargs['default'] = kwargs.get('default', [])
        super(FieldsetField, self).__init__(*args, **kwargs)

    def clean(self, new_value, old_value, required, trim_on_empty, base_initial, base_data):
        if new_value and not isinstance(new_value, dict):
            raise ValidationError(
                'Value should be a dict',
                code='should_be_a_dict',
            )
        if required and not new_value:
            raise ValidationError('This field is required.', code='required')

        if trim_on_empty and not new_value:
            raise ValidationError('Avoid writing to cleaned_data ', code='empty_value')

        sform = self.sform_cls(
            data=new_value,
            initial=old_value,
            base_initial=base_initial,
            base_data=base_data,
        )
        if sform._get_errors():
            raise ValidationError(sform._get_errors())
        return sform.cleaned_data

    def data_as_dict(self, **kwargs):
        field_dict = super(FieldsetField, self).data_as_dict(**kwargs)
        sform = self.sform_cls(
            initial=kwargs['value'],
            base_data=kwargs['base_data'],
            base_initial=kwargs['base_initial'],
        )
        field_dict['value'] = sform.data_as_dict(prefix=field_dict['name'])
        return field_dict

    def structure_as_dict(self, **kwargs):
        field_dict = super(FieldsetField, self).structure_as_dict(**kwargs)
        sform = self.sform_cls(
            base_data=kwargs['base_data'],
            base_initial=kwargs['base_initial'],
        )
        field_dict['value'] = sform.structure_as_dict(prefix=field_dict['name'])
        return field_dict


def _extract_errors(name, e):
    if hasattr(e, 'error_dict'):
        errors = e.error_dict.items()
    else:
        errors = [(tuple(), e.error_list)]

    field_key = tuple() if name is None else (name,)
    for key, value in errors:
        key = field_key + key
        yield key, value


def _default_values_matcher(old_values, new_values, default):
    for index, new_value in enumerate(new_values):
        old_value = old_values[index] if index < len(old_values) else default
        yield old_value, new_value


class GridField(Field):
    def __init__(self, field_instance, values_matcher=None, *args, **kwargs):
        self.field_instance = field_instance
        kwargs['default'] = kwargs.get('default', [])
        self.values_matcher = values_matcher or _default_values_matcher
        super(GridField, self).__init__(**kwargs)

    def clean(self, new_value, old_value, required, trim_on_empty, base_initial, base_data):
        if not isinstance(new_value, list):
            raise ValidationError(
                'Value should be a list',
                code='should_be_a_list',
            )
        if required and not new_value:
            raise ValidationError('This field is required.', code='required')

        if trim_on_empty and not new_value:
            raise ValidationError('Avoid writing to cleaned_data ', code='empty_value')

        error_dict = {}
        cleaned_data = []
        value_pairs = self.values_matcher(
            old_value,
            new_value,
            self.field_instance.default
        )
        for index, (old_value_item, new_value_item) in enumerate(value_pairs):
            try:
                cd = self.field_instance.clean(
                    new_value=new_value_item,
                    old_value=old_value_item,
                    required=required,
                    trim_on_empty=trim_on_empty,
                    base_initial=base_initial,
                    base_data=base_data,
                )
            except ValidationError as e:
                error_dict.update(_extract_errors(name=index, e=e))
            else:
                cleaned_data.append(cd)

        if error_dict:
            raise ValidationError(error_dict)

        return cleaned_data

    def data_as_dict(self, **kwargs):
        field_dict = super(GridField, self).data_as_dict(**kwargs)
        fieldset_field = self.field_instance
        field_dict['value'] = [
            fieldset_field.data_as_dict(
                prefix=field_dict['name'],
                name=smart_text(n),
                value=value,
                state=kwargs['state'],
                base_initial=kwargs['base_initial'],
                base_data=kwargs['base_data'],
            )
            for n, value in enumerate(kwargs['value'])
        ]
        return field_dict

    def structure_as_dict(self, **kwargs):
        field_dict = super(GridField, self).structure_as_dict(**kwargs)
        field_dict['structure'] = self.field_instance.structure_as_dict(
            prefix=field_dict['name'],
            name='0',
            state=kwargs['state'],
            base_initial=kwargs['base_initial'],
            base_data=kwargs['base_data'],
        )
        return field_dict


class SFormMetaclass(type):
    def __new__(mcs, name, bases, attrs):
        fields = []
        for field_name, obj in attrs.items():
            if isinstance(obj, Field):
                fields.append((field_name, obj))
        for field_name, obj in fields:
            del attrs[field_name]
        fields.sort(key=lambda x: x[1].creation_counter)

        for base in bases[::-1]:
            if hasattr(base, 'base_fields'):
                fields = list(base.base_fields.items()) + fields

        attrs['base_fields'] = OrderedDict(fields)

        new_class = super(
            SFormMetaclass, mcs).__new__(mcs, name, bases, attrs)

        return new_class


class BaseSForm(object):
    default_getter = operator.itemgetter

    def __init__(self, data=None, initial=None, base_data=None, base_initial=None):
        self.data = data or {}
        self.initial = initial or {}

        self.base_initial = self.initial if base_initial is None else base_initial
        self.base_data = self.data if base_data is None else base_data

        self._errors = None
        self.cleaned_data = {}

        self.fields = copy.deepcopy(self.base_fields)
        self.fields_state = self.prepare_fields_state()

    def prepare_fields_state(self):
        return {name: f.state for name, f in self.fields.items()}

    def _get_initial_value(self, name, field):
        getter = getattr(self, 'get_{name}'.format(name=name), None)

        if getter is None:
            try:
                value = self.default_getter(name)(self.initial)
            except (KeyError, AttributeError, IndexError):
                value = field.default
        else:
            value = getter(self.initial)
        return value

    def _field_is_readonly(self, field_state, name, old_value):
        readonly = field_state == READONLY
        if readonly:
            if name in self.initial:
                self.cleaned_data[name] = old_value
        return readonly

    def _check_field_custom_clean(self, name, value):
        if hasattr(self, 'clean_%s' % name):
            value = getattr(self, 'clean_%s' % name)(value)
            self.cleaned_data[name] = value

    def _handle_field_exception(self, name, e):
        if hasattr(e, 'code') and e.code == 'empty_value':
            return
        self._errors.update(_extract_errors(name=name, e=e))
        if name in self.cleaned_data:
            del self.cleaned_data[name]

    def _clean_fields(self):
        self._errors = {}
        for name, field in self.fields.items():
            field_state = self.get_field_state(name)

            old_value = self._get_initial_value(name, field)
            if self._field_is_readonly(field_state, name, old_value):
                continue

            new_value = field.extract_new_value(self.data, name)

            try:
                value = field.clean(
                    new_value=new_value,
                    old_value=old_value,
                    required=(field_state == REQUIRED),
                    trim_on_empty=field.trim_on_empty,
                    base_initial=self.base_initial,
                    base_data=self.base_data,
                )
                self.cleaned_data[name] = value
                self._check_field_custom_clean(name, value)
            except ValidationError as e:
                self._handle_field_exception(name, e)

    def _get_errors(self):
        if self._errors is None:
            self._clean_fields()
            try:
                self.cleaned_data = self.clean()
            except ValidationError as e:
                self._errors.update(_extract_errors(name=None, e=e))
        return self._errors

    def errors_as_dict(self):
        def error_as_dict(err):
            error = {'code': err.code}
            if err.params:
                error['params'] = err.params
            return error

        errors = {}
        for key, value in self._errors.items():
            if key:
                key = key[0] + ''.join('[%s]' % k for k in key[1:])
            else:
                key = ''
            errors[key] = [error_as_dict(e) for e in value]
        return {'errors': errors}

    errors = property(errors_as_dict)

    def is_valid(self):
        return not bool(self._get_errors())

    def clean(self):
        return self.cleaned_data

    def get_field_state(self, name):
        return self.fields_state[name]

    def as_dict(self):
        result = {
            'structure': self.structure_as_dict()
        }
        if self.initial or self.data:
            result['data'] = self.data_as_dict()
        return result

    def data_as_dict(self, prefix=''):
        result = OrderedDict()
        for name, field in self.fields.items():
            value = self._get_initial_value(name, field)
            state = self.get_field_state(name)
            result[name] = field.data_as_dict(
                prefix=prefix,
                name=name,
                value=value,
                state=state,
                base_data=self.base_data,
                base_initial=self.base_initial,
            )
        return result

    def structure_as_dict(self, prefix=''):
        result = OrderedDict()
        for name, field in self.fields.items():
            state = self.get_field_state(name)
            result[name] = field.structure_as_dict(
                prefix=prefix,
                name=name,
                state=state,
                base_data=self.base_data,
                base_initial=self.base_initial,
            )
        return result


class SForm(six.with_metaclass(SFormMetaclass, BaseSForm)):
    "SForm"
