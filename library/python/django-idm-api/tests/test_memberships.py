# coding: utf-8
from __future__ import unicode_literals
import json
import pytest
import collections
try:
    from django.urls import reverse
except ImportError:
    from django.core.urlresolvers import reverse
from django.test.utils import override_settings
from django_idm_api.hooks import BaseHooks, RoleStream

pytestmark = pytest.mark.django_db


class CustomQuerySet(object):
    def __init__(self, values):
        # values - list or collections.defaultdict(list)
        if isinstance(values, list):
            self.from_list(values)
        elif isinstance(values, collections.defaultdict):
            self.from_defaultdict(values)
        else:
            raise ValueError

    def from_list(self, values):
        self.values = values

    def from_defaultdict(self, values):
        temp = []
        i = 0
        for key, items in values.items():
            for item in items:
                i += 1
                item['group'] = key
                item['pk'] = i
                temp.append(item)
        self.values = temp

    def __getitem__(self, item):
        if isinstance(item, slice):
            return self.values[item.start:item.stop:item.step]
        return self.values[item]

    def aggregate(self, *args, **kwargs):
        return self

    def get(self, *args, **kwargs):
        if self.values:
            return self.values[-1]['pk']

    def filter(self, pk__gt, *args, **kwargs):
        return [item for item in self.values if item[0] > pk__gt]


class CustomGroupMembershipStream(RoleStream):
    name = 'group_memberships'

    def get_queryset(self):
        return CustomQuerySet(CustomHooks.group_memberships)

    def values_list(self, queryset):
        return CustomQuerySet([(item['pk'], item['login'], item['group'], item['passport_login']) for item in queryset])

    def row_as_dict(self, row):
        pk, username, group, passport_login = row
        return {
            'login': username,
            'group': group,
            'passport_login': passport_login,
        }


class CustomHooks(BaseHooks):
    GET_MEMBERSHIP_PAGE_SIZE = 1
    GET_MEMBERSHIP_STREAMS = [CustomGroupMembershipStream]
    group_memberships = collections.defaultdict(list)

    def add_membership_impl(self, login, group_external_id, passport_login, **kwargs):
        CustomHooks.group_memberships[group_external_id].append({'login': login, 'passport_login': passport_login})
        return self._successful_result

    def remove_membership_impl(self, login, group_external_id, **kwargs):
        if group_external_id not in CustomHooks.group_memberships:
            return self._successful_result
        CustomHooks.group_memberships[group_external_id] = [
            membership
            for membership
            in CustomHooks.group_memberships[group_external_id]
            if membership['login'] != login
        ]
        return self._successful_result


@pytest.fixture
def clean_customhooks():
    CustomHooks.group_memberships = collections.defaultdict(list)


def test_add_membership(client, frodo, clean_customhooks):
    # Проверим добавление пользователя в группу с использованием паспортного логина
    url = reverse('client-api:add-membership')
    with override_settings(ROLES_HOOKS=CustomHooks):
        response = client.json.post(url, {
            'login': 'frodo',
            'group': 10,
            'passport_login': 'yndx-frodo@yandex-team.ru',
        })
    assert response.status_code == 200
    assert response.json() == {'code': 0}
    assert len(CustomHooks.group_memberships) == 1
    assert len(CustomHooks.group_memberships[10]) == 1
    assert CustomHooks.group_memberships[10] == [{'login': 'frodo', 'passport_login': 'yndx-frodo@yandex-team.ru'}]

    # Проверим добавление пользователя в группу без паспортного логина
    with override_settings(ROLES_HOOKS=CustomHooks):
        response = client.json.post(url, {
            'login': 'frodo',
            'group': 20,
        })
    assert response.status_code == 200
    assert response.json() == {'code': 0}
    assert len(CustomHooks.group_memberships) == 2
    assert len(CustomHooks.group_memberships[20]) == 1
    assert CustomHooks.group_memberships[20] == [{'login': 'frodo', 'passport_login': ''}]


def test_remove_membership(client, frodo, clean_customhooks):
    url = reverse('client-api:add-membership')
    with override_settings(ROLES_HOOKS=CustomHooks):
        response = client.json.post(url, {
            'login': 'frodo',
            'group': 10,
            'passport_login': 'yndx-frodo@yandex-team.ru',
        })
    assert response.status_code == 200

    url = reverse('client-api:remove-membership')

    with override_settings(ROLES_HOOKS=CustomHooks):
        response = client.json.post(url, {
            'login': 'frodo',
            'group': 10,
        })
    assert response.status_code == 200
    assert response.json() == {'code': 0}
    assert len(CustomHooks.group_memberships) == 1
    assert len(CustomHooks.group_memberships[10]) == 0


def test_add_membership_bad_request(client, frodo, clean_customhooks):
    url = reverse('client-api:add-membership')

    # TypeError: Cannot encode None for key 'fired' as POST data. Did you mean to pass an empty string or omit the value?
    # The reason is that the default content type (multipart/form-data) doesn't support null values, only empty strings, hence the suggestion.
    response = client.json.post(url, {
        'login': 'unknown-login',
        'group': 'None'
    })
    assert response.status_code == 200
    assert response.json() == {
        'code': 400,
        'fatal': u'Enter a whole number.',
    }

    response = client.json.post(url, {
        'login': 'unknown-login',
        'group': 'ten'
    })
    assert response.status_code == 200
    assert response.json() == {
        'code': 400,
        'fatal': u'Enter a whole number.',
    }

    response = client.json.post(url, {
        'login': 'unknown-login'
    })
    assert response.status_code == 200
    assert response.json() == {
        'code': 400,
        'fatal': u'Field "group" is required.',
    }


def test_add_batch_memberships(client, frodo, clean_customhooks):
    # Проверим добавление пользователей в batch-режиме с валидными данными
    url = reverse('client-api:add-batch-memberships')
    data = [
        {
            'login': 'frodo',
            'group': 10,
            'passport_login': 'yndx-frodo@yandex-team.ru',
        },
        {
            'login': 'frodo',
            'group': 15,
        }
    ]

    with override_settings(ROLES_HOOKS=CustomHooks):
        response = client.json.post(url, {'data': json.dumps(data)})
    assert response.status_code == 200

    assert response.json() == {'code': 0}
    assert len(CustomHooks.group_memberships) == 2
    assert len(CustomHooks.group_memberships[10]) == 1
    assert CustomHooks.group_memberships[10] == [{'login': 'frodo', 'passport_login': 'yndx-frodo@yandex-team.ru'}]
    assert len(CustomHooks.group_memberships[15]) == 1
    assert CustomHooks.group_memberships[15] == [{'login': 'frodo', 'passport_login': ''}]


def test_remove_batch_memberships(client, frodo, clean_customhooks):
    url = reverse('client-api:add-batch-memberships')
    data = [
        {
            'login': 'frodo',
            'group': 10,
            'passport_login': 'yndx-frodo@yandex-team.ru',
        },
        {
            'login': 'frodo',
            'group': 15,
        }
    ]

    with override_settings(ROLES_HOOKS=CustomHooks):
        response = client.json.post(url, {'data': json.dumps(data)})
    assert response.status_code == 200

    url = reverse('client-api:remove-batch-memberships')

    with override_settings(ROLES_HOOKS=CustomHooks):
        response = client.json.post(url, {'data': json.dumps(data[:1])})
    assert response.status_code == 200
    assert response.json() == {'code': 0}
    assert len(CustomHooks.group_memberships) == 2
    assert len(CustomHooks.group_memberships[10]) == 0


@pytest.mark.parametrize('viewname', ['client-api:add-batch-memberships', 'client-api:remove-batch-memberships'])
def test_batch_memberships_bad_requests(client, frodo, clean_customhooks, viewname):
    # Проверим добавление пользователей в batch-режиме с невалидными данными
    url = reverse(viewname)
    data = [
        {
            'login': 'frodo',
            'group': 'ten',
            'passport_login': 'yndx-frodo@yandex-team.ru',
        },
        {
            'login': 'frodo',
        },
        {
            'group': 10,
            'passport_login': 'yndx-frodo@yandex-team.ru',
        },
    ]

    with override_settings(ROLES_HOOKS=CustomHooks):
        response = client.json.post(url, {'data': json.dumps(data)})
    assert response.status_code == 200

    expected_data = {
        'code': 207,
        'multi_status': [
            {
                'login': 'frodo',
                'group': 'ten',
                'passport_login': 'yndx-frodo@yandex-team.ru',
                'error': 'Enter a whole number.',
            },
            {
                'login': 'frodo',
                'error': 'Field "group" is required.',
            },
            {
                'passport_login': 'yndx-frodo@yandex-team.ru',
                'group': 10,
                'error': 'Field "login" is required.'
            }
        ],
    }

    assert response.json() == expected_data
    assert len(CustomHooks.group_memberships) == 0


def test_get_memberships(client, frodo, clean_customhooks):
    url = reverse('client-api:add-membership')
    with override_settings(ROLES_HOOKS=CustomHooks):
        client.json.post(url, {
            'login': 'frodo',
            'group': 10,
            'passport_login': 'yndx-frodo@yandex-team.ru',
        })

        client.json.post(url, {
            'login': 'frodo',
            'group': 20,
        })

        response = client.json.get(reverse('client-api:get-memberships'))
        assert response.status_code == 200
        data = response.json()
        assert data == {
            'memberships': [{
                'login': 'frodo',
                'group': 10,
                'passport_login': 'yndx-frodo@yandex-team.ru'
            }],
            'code': 0,
            'next-url': '/client-api/get-memberships/?type=group_memberships&since=1',
        }

        response = client.json.get(data['next-url'])
        assert response.status_code == 200
        data = response.json()
        assert data == {
            'memberships': [{
                'login': 'frodo',
                'group': 20,
                'passport_login': '',
            }],
            'code': 0,
        }
