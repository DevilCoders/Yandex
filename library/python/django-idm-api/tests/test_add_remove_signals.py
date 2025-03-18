# coding: utf-8
from __future__ import unicode_literals

import pytest

try:
    from django.urls import reverse
except ImportError:
    from django.core.urlresolvers import reverse
from django.dispatch import receiver
from django.test.utils import override_settings

from django_idm_api.hooks import BaseHooks
from django_idm_api.signals import role_added, role_removed
from django_idm_api.tests.utils import create_group

pytestmark = pytest.mark.django_db


def test_add_role(client, frodo):
    signal_info = {'data': None}

    @receiver(role_added)
    def signal_handler(sender, **info):
        signal_info['data'] = info
        del signal_info['data']['signal']

    url = reverse('client-api:add-role')

    response = client.json.post(url, {
        'login': 'frodo',
        'role': {
            'role': 'superuser'
        }
    })

    assert response.status_code == 200

    assert signal_info['data'] == {
        'login': 'frodo',
        'role': {
            'role': 'superuser'
        },
        'fields': {}
    }


@pytest.mark.parametrize('role', ['gibberish', {'role': None}])
def test_add_role_bad_request(client, role):
    signal_info = {'data': None}

    @receiver(role_added)
    def signal_handler(sender, **info):
        signal_info['data'] = info
        del signal_info['data']['signal']

    url = reverse('client-api:add-role')
    # bad json request
    client.json.post(url, {
        'login': 'unknown-login',
        'role': role
    })

    assert signal_info['data'] is None


def test_add_group_role_if_system_is_aware(client):
    signal_info = {'data': None}

    @receiver(role_added)
    def signal_handler(sender, **info):
        signal_info['data'] = info
        del signal_info['data']['signal']

    url = reverse('client-api:add-role')

    class CustomHooks(BaseHooks):
        role_is_added = None

        def add_group_role_impl(self, group, role, fields, **kwargs):
            CustomHooks.role_is_added = True
            CustomHooks.group = group
            CustomHooks.role = role
            CustomHooks.fields = fields
            return self._successful_result

    with override_settings(ROLES_HOOKS=CustomHooks, ROLES_SYSTEM_IS_GROUP_AWARE=True):
        client.json.post(url, {
            'group': 13,
            'role': {
                'role': 'superuser'
            },
            'fields': {
                'token': 'mellon'
            }
        })
        assert signal_info['data'] == {
            'group': 13,
            'role': {
                'role': 'superuser'
            },
            'fields': {
                'token': 'mellon'
            }
        }


def test_deprive_role(client, frodo):
    signal_info = {'data': None}

    @receiver(role_removed)
    def signal_handler(sender, **info):
        signal_info['data'] = info
        del signal_info['data']['signal']

    url = reverse('client-api:remove-role')

    group = create_group('Менеджеры')
    frodo.groups.add(group)
    frodo.is_staff = True
    frodo.save()

    client.json.post(url, {
        'login': 'frodo',
        'role': {
            'role': 'group-%d' % group.pk
        }
    })

    assert signal_info['data'] == {
        'login': 'frodo',
        'role': {
            'role': 'group-%d' % group.pk
        },
        'data': {},
        'is_fired': False
    }


def test_deprive_unknown_role(client):
    signal_info = {'data': None}

    @receiver(role_removed)
    def signal_handler(sender, **info):
        signal_info['data'] = info
        del signal_info['data']['signal']

    url = reverse('client-api:remove-role')
    client.json.post(url, {
        'login': 'unknown-login',
        'role': {
            'bad': 'role'
        }
    })

    assert signal_info['data'] is None


@pytest.mark.parametrize('role', ['gibberish', {'role': None}])
def test_deprive_role_bad_req(client, role):
    signal_info = {'data': None}

    @receiver(role_removed)
    def signal_handler(sender, **info):
        signal_info['data'] = info
        del signal_info['data']['signal']

    url = reverse('client-api:remove-role')

    # bad json request
    client.json.post(url, {
        'login': 'unknown-login',
        'role': role
    })

    assert signal_info['data'] is None


def test_deprive_group_role_if_system_is_aware(client):
    """Если включена соответствующая настройка, то система сама следит за составом групп.
    Мы (пока?) не предоставляем готового механизма для отслеживания состава групп, так что хуки тоже
    будут переопределены"""

    signal_info = {'data': None}

    @receiver(role_removed)
    def signal_handler(sender, **info):
        signal_info['data'] = info
        del signal_info['data']['signal']

    url = reverse('client-api:remove-role')

    class CustomHooks(BaseHooks):
        role_is_removed = None

        def remove_group_role_impl(self, group, role, data, is_deleted, **kwargs):
            CustomHooks.role_is_removed = True
            CustomHooks.group = group
            CustomHooks.role = role
            CustomHooks.data = data
            CustomHooks.is_deleted = is_deleted
            return self._successful_result

    with override_settings(ROLES_HOOKS=CustomHooks, ROLES_SYSTEM_IS_GROUP_AWARE=True):
        client.json.post(url, {
            'group': 13,
            'role': {
                'role': 'superuser'
            },
            'data': {
                'token': 'mellon'
            }
        })

        assert signal_info['data'] == {
            'group': CustomHooks.group,
            'role': CustomHooks.role,
            'data': CustomHooks.data,
            'is_deleted': CustomHooks.is_deleted
        }
