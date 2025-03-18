# coding: utf-8
from __future__ import unicode_literals

import json

from django import forms
from django.core.validators import EMPTY_VALUES
from django_idm_api.exceptions import BadRequest
from django_idm_api.constants import SUBJECT_TYPES
from django_idm_api.utils import is_b2b


class SelfAwareMixin(object):
    default_error_messages = {
        'required': 'Field "%(name)s" is required.'
    }

    def __init__(self, **kwargs):
        self.name = kwargs.pop('name')
        super(SelfAwareMixin, self).__init__(**kwargs)

    def validate(self, value):
        if value in EMPTY_VALUES and self.required:
            raise forms.ValidationError(self.error_messages['required'] % {'name': self.name})


class JSONField(SelfAwareMixin, forms.Field):
    def clean(self, value):
        if not self.required and value is None:
            value = {}
        else:
            try:
                value = json.loads(value)
            except (ValueError, TypeError):
                raise forms.ValidationError('incorrect json data in field `%(field)s`: %(value)s' % {
                    'field': self.name,
                    'value': value
                })
        return value


class BooleanField(forms.Field):
    def clean(self, value):
        return bool(value and value == '1')


class SelfAwareCharField(SelfAwareMixin, forms.CharField):
    """Поле для более детальных сообщений об ошибках"""


class SelfAwareIntegerField(SelfAwareMixin, forms.IntegerField):
    """Поле для более детальных сообщений об ошибках"""


class InternalForm(forms.Form):
    def get_clean_data(self):
        data = [self.cleaned_data[key] for key in self.fields.keys()]
        return data

    def raise_first(self):
        for fieldname in self.fields.keys():
            if fieldname in self._errors:
                raise BadRequest(self._errors[fieldname][0])
        if len(self.non_field_errors()) >= 1:
            raise BadRequest(self.non_field_errors()[0])


class B2BFormMixin(object):
    org_id = SelfAwareIntegerField(name='org_id', required=True, error_messages={
        'invalid': 'Field "organization" must be an integer'
    })

    def __init__(self, *args, **kwargs):
        super(InternalForm, self).__init__(*args, **kwargs)
        if is_b2b():
            self.add_org_id_field()
            if 'login' in self.fields:
                self.change_login_to_uid()

    def add_org_id_field(self):
        self.fields['org_id'] = SelfAwareIntegerField(
            name='org_id',
            required=True,
            error_messages={'invalid': 'Field "organization" must be an integer'},
        )

    def change_login_to_uid(self):
        self.fields.pop('login')
        self.fields['uid'] = SelfAwareCharField(name='uid', required=True)


class AddUserRoleForm(B2BFormMixin, InternalForm):
    login = SelfAwareCharField(name='login', required=True)
    role = JSONField(name='role', required=True)
    fields = JSONField(name='fields', required=False)
    subject_type = forms.ChoiceField(
        required=False,
        choices=SUBJECT_TYPES.CHOICES,
        initial=SUBJECT_TYPES.USER,
    )


class RemoveUserRoleForm(B2BFormMixin, InternalForm):
    login = SelfAwareCharField(name='login', required=True)
    role = JSONField(name='role', required=True)
    fields = JSONField(name='fields', required=False)
    fired = BooleanField()
    subject_type = forms.ChoiceField(
        required=False,
        choices=SUBJECT_TYPES.CHOICES,
        initial=SUBJECT_TYPES.USER,
    )


class AddGroupRoleForm(B2BFormMixin, InternalForm):
    group = SelfAwareIntegerField(name='group', required=True, error_messages={
        'invalid': 'Field "group" must be an integer'
    })
    role = JSONField(name='role', required=True)
    fields = JSONField(name='fields', required=False)
    subject_type = forms.ChoiceField(
        required=False,
        choices=SUBJECT_TYPES.CHOICES,
        initial=SUBJECT_TYPES.USER,
    )


class RemoveGroupRoleForm(B2BFormMixin, InternalForm):
    group = SelfAwareIntegerField(name='group', required=True, error_messages={
        'invalid': 'Field "group" must be an integer'
    })
    role = JSONField(name='role', required=True)
    fields = JSONField(name='fields', required=False)
    is_deleted = BooleanField()
    subject_type = forms.ChoiceField(
        required=False,
        choices=SUBJECT_TYPES.CHOICES,
        initial=SUBJECT_TYPES.USER,
    )


class GetRolesForm(InternalForm):
    since = forms.IntegerField(required=False)
    type = forms.ChoiceField(required=False)

    def __init__(self, params, hooks):
        super(GetRolesForm, self).__init__(params)
        types = [stream.name for stream in hooks.GET_ROLES_STREAMS]
        self.fields['type'].choices = zip(types, types)
        self.hooks = hooks

    def get_clean_data(self):
        stream = self.cleaned_data.get('type') or self.hooks.GET_ROLES_STREAMS[0].name
        since = self.cleaned_data.get('since')
        return stream, since


class GetMembershipsForm(InternalForm):
    since = forms.IntegerField(required=False)
    type = forms.ChoiceField(required=False)

    def __init__(self, params, hooks):
        super(GetMembershipsForm, self).__init__(params)
        types = [stream.name for stream in hooks.GET_MEMBERSHIP_STREAMS]
        self.fields['type'].choices = zip(types, types)
        self.hooks = hooks

    def get_clean_data(self):
        stream = self.cleaned_data.get('type') or self.hooks.GET_MEMBERSHIP_STREAMS[0].name
        since = self.cleaned_data.get('since')
        return stream, since


class AddMembershipForm(InternalForm):
    login = SelfAwareCharField(name='login', required=True)
    group = SelfAwareIntegerField(name='group', required=True)
    passport_login = SelfAwareCharField(name='passport_login', required=False)


class RemoveMembershipForm(InternalForm):
    login = SelfAwareCharField(name='login', required=True)
    group = SelfAwareIntegerField(name='group', required=True)
