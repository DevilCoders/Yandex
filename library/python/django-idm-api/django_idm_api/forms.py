# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import json
from django import forms
from django.contrib.auth.models import Group
from django.core.exceptions import ValidationError
from django.utils.functional import lazy

from django_idm_api.utils import get_hooks
from django_idm_api.compat import get_user_model


class CreativeModelChoiceField(forms.ModelChoiceField):
    """Это поле умеет создавать нужный объект, если его нет в queryset."""
    def __init__(self, *args, **kwargs):
        self.create_if_doesnotexist = False
        self.create_object = lambda **kwargs: self.queryset.create(**kwargs)
        super(CreativeModelChoiceField, self).__init__(*args, **kwargs)

    def to_python(self, value):
        if self.create_if_doesnotexist:
            key = self.to_field_name or 'pk'
            params = {key: value}
            try:
                self.queryset.get(**params)
            except self.queryset.model.DoesNotExist:
                self.create_object(**params)
        return super(CreativeModelChoiceField, self).to_python(value)


class RoleField(forms.ModelChoiceField):
    def __init__(self, *args, **kwargs):
        self.to_field_name = kwargs.get('to_field_name', None)
        super(RoleField, self).__init__(*args, **kwargs)

    def _get_choices(self):
        hooks = get_hooks()
        return hooks.info()['roles']['values'].items()

    choices = property(lazy(_get_choices), forms.ModelChoiceField._set_choices)

    def to_python(self, value):
        value = json.loads(value)['role']

        if value.startswith('group-'):
            value = value[6:]
            return super(RoleField, self).to_python(value)

        if value not in map(lambda x: x[0], self.choices):
            raise ValidationError(self.error_messages['invalid_choice'])

        return value


class RoleForm(forms.Form):
    login = CreativeModelChoiceField(
        queryset=get_user_model()._default_manager,
        to_field_name='username',
    )
    role = RoleField(queryset=Group._default_manager)


class LoginForm(forms.Form):
    login = forms.ModelChoiceField(
        queryset=get_user_model()._default_manager,
        to_field_name='username',
    )
