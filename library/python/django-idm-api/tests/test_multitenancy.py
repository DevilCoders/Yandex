# coding: utf-8
from __future__ import unicode_literals

try:
    from django.urls import reverse
except ImportError:
    from django.core.urlresolvers import reverse

from django_idm_api.hooks import AuthHooks

from django_idm_api.tests.utils import create_group, create_user

import pytest


pytestmark = pytest.mark.django_db


class SystemAuthHooks(AuthHooks):
    def __init__(self, system):
        self.system = system

    def get_group_queryset(self):
        return super(SystemAuthHooks, self).get_group_queryset().filter(name__startswith='%s-' % self.system)


@pytest.fixture(autouse=True)
def system_auth_hooks(settings):
    settings.ROLES_HOOKS = SystemAuthHooks


def test_info_multitenant(client):
    url = reverse('system-client-api:info', kwargs={'system': 'foo'})

    group = create_group('foo-manager')
    create_group('bar-manager')

    response = client.json.get(url)

    assert response.status_code == 200
    assert set(response.json()['roles']['values'].keys()) == {'group-%s' % group.id, 'superuser'}


def test_add_remove_role_multitenant(client, frodo):
    url = reverse('system-client-api:add-role', kwargs={'system': 'foo'})

    group = create_group('foo-manager')
    another_group = create_group('bar-manager')

    response = client.json.post(url, {
        'login': 'frodo',
        'role': {
            'role': 'group-%s' % group.id
        }
    })
    assert response.status_code == 200
    assert response.json() == {
        'code': 0
    }
    assert group.user_set.filter(id=frodo.id).exists()
    assert not another_group.user_set.exists()

    url = reverse('system-client-api:remove-role', kwargs={'system': 'foo'})

    response = client.json.post(url, {
        'login': 'frodo',
        'role': {
            'role': 'group-%s' % group.id
        }
    })
    assert response.status_code == 200
    assert response.json() == {
        'code': 0
    }
    assert not group.user_set.filter(id=frodo.id).exists()
    assert not another_group.user_set.exists()


def test_get_roles_multitenant(client):
    foo_manager = create_group('foo-manager')
    bar_manager = create_group('bar-manager')

    aragorn = create_user('aragorn')
    aragorn.groups.add(foo_manager)
    sam = create_user('sam')
    sam.groups.add(foo_manager)

    barlog = create_user('barlog')
    barlog.groups.add(bar_manager)

    url = reverse('system-client-api:get-all-roles', kwargs={'system': 'foo'})
    response = client.json.get(url)
    assert response.json() == {
        'code': 0,
        'users': [
            {
                'login': 'aragorn',
                'roles': [{'role': 'group-%d' % foo_manager.id}]
            },
            {
                'login': 'sam',
                'roles': [{'role': 'group-%d' % foo_manager.id}]
            }
        ]
    }

    url = reverse('system-client-api:get-roles', kwargs={'system': 'bar'})
    response = client.json.get(url, {'type': 'memberships'})
    assert response.json() == {
        'code': 0,
        'roles': [
            {'login': 'barlog', 'path': '/role/group-%d/' % bar_manager.id},
        ]
    }
