# coding: utf-8
from __future__ import unicode_literals

import pytest
import mock
try:
    from django.urls import reverse
except ImportError:
    from django.core.urlresolvers import reverse
from django_idm_api.tests.utils import create_group, create_user, create_superuser

pytestmark = pytest.mark.django_db


def test_empty(client):
    url = reverse('client-api:get-roles')
    response = client.json.get(url)
    assert response.json() == {
        'code': 0,
        'roles': [],
        'next-url': url + '?type=memberships'
    }
    response = client.json.get(url, {'type': 'memberships'})
    assert response.json() == {
        'code': 0,
        'roles': []
    }


def test_superuser(client):
    create_superuser('admin')

    url = reverse('client-api:get-roles')
    response = client.json.get(url)
    assert response.json() == {
        'code': 0,
        'roles': [{
            'login': 'admin',
            'path': '/role/superuser/'
        }],
        'next-url': url + '?type=memberships'
    }

    response = client.json.get(url, {'type': 'memberships'})
    assert response.json() == {
        'code': 0,
        'roles': []
    }


def test_two_superusers(client):
    create_superuser('first')
    create_superuser('second')

    url = reverse('client-api:get-roles')
    response = client.json.get(url)
    assert response.json() == {
        'code': 0,
        'roles': [{
            'login': 'first',
            'path': '/role/superuser/'
        }, {
            'login': 'second',
            'path': '/role/superuser/'
        }],
        'next-url': url + '?type=memberships'
    }


def test_superuser_pagination(client):
    first = create_superuser('first')
    create_superuser('second')

    with mock.patch('django_idm_api.hooks.AuthHooks.GET_ROLES_PAGE_SIZE', 1):
        url = reverse('client-api:get-roles')
        response = client.json.get(url)
        next_url = url + '?type=superusers&since=%d' % first.pk
        assert response.json() == {
            'code': 0,
            'roles': [{
                'login': 'first',
                'path': '/role/superuser/'
            }],
            'next-url': next_url
        }
        response = client.json.get(url, {'type': 'superusers', 'since': first.pk})
        assert response.json() == {
            'code': 0,
            'roles': [{
                'login': 'second',
                'path': '/role/superuser/'
            }],
            'next-url': url + '?type=memberships'
        }


def test_memberships(client):
    aragorn = create_user('aragorn')
    barlog = create_user('barlog')

    manager = create_group('Менеджер')
    manager_path = '/role/group-%d/' % manager.pk
    admin = create_group('Админ')
    admin_path = '/role/group-%d/' % admin.pk

    aragorn.groups.add(manager)
    barlog.groups.add(admin)

    url = reverse('client-api:get-roles')
    response = client.json.get(url)
    assert response.json() == {
        'code': 0,
        'roles': [],
        'next-url': url + '?type=memberships'
    }
    response = client.json.get(url, {'type': 'memberships'})
    assert response.json() == {
        'code': 0,
        'roles': [{
            'login': 'aragorn',
            'path': manager_path
        }, {
            'login': 'barlog',
            'path': admin_path
        }]
    }


def test_memberships_paging(client):
    aragorn = create_user('aragorn')
    barlog = create_user('barlog')

    first = create_superuser('first')
    create_superuser('second')

    manager = create_group('Менеджер')
    manager_path = '/role/group-%d/' % manager.pk
    admin = create_group('Админ')
    admin_path = '/role/group-%d/' % admin.pk

    aragorn.groups.add(manager)
    membership_id = aragorn._meta.get_field('groups').remote_field.through.objects.get().pk
    barlog.groups.add(admin)

    with mock.patch('django_idm_api.hooks.AuthHooks.GET_ROLES_PAGE_SIZE', 1):
        url = reverse('client-api:get-roles')
        response = client.json.get(url)
        next_url = url + '?type=superusers&since=%d' % first.pk
        assert response.json() == {
            'code': 0,
            'roles': [{
                'login': 'first',
                'path': '/role/superuser/'
            }],
            'next-url': next_url
        }
        response = client.json.get(url, {'type': 'superusers', 'since': first.pk})
        assert response.json() == {
            'code': 0,
            'roles': [{
                'login': 'second',
                'path': '/role/superuser/'
            }],
            'next-url': url + '?type=memberships'
        }
        response = client.json.get(url, {'type': 'memberships'})
        assert response.json() == {
            'code': 0,
            'roles': [{
                'login': 'aragorn',
                'path': manager_path
            }],
            'next-url': url+'?type=memberships&since=%d' % membership_id
        }
        response = client.json.get(url, {'type': 'memberships', 'since': membership_id})
        assert response.json() == {
            'code': 0,
            'roles': [{
                'login': 'barlog',
                'path': admin_path
            }]
        }


def test_everything(client):
    aragorn = create_user('aragorn')
    barlog = create_user('barlog')

    first = create_superuser('first')
    create_superuser('second')

    manager = create_group('Менеджер')
    manager_path = '/role/group-%d/' % manager.pk
    admin = create_group('Админ')
    admin_path = '/role/group-%d/' % admin.pk

    aragorn.groups.add(manager)
    membership_id = aragorn._meta.get_field('groups').remote_field.through.objects.get().pk
    barlog.groups.add(admin)

    with mock.patch('django_idm_api.hooks.AuthHooks.GET_ROLES_PAGE_SIZE', 1):
        url = reverse('client-api:get-roles')
        response = client.json.get(url)
        next_url = url + '?type=superusers&since=%d' % first.pk
        assert response.json() == {
            'code': 0,
            'roles': [{
                'login': 'first',
                'path': '/role/superuser/'
            }],
            'next-url': next_url
        }
        response = client.json.get(url, {'type': 'superusers', 'since': first.pk})
        assert response.json() == {
            'code': 0,
            'roles': [{
                'login': 'second',
                'path': '/role/superuser/'
            }],
            'next-url': url + '?type=memberships'
        }
        response = client.json.get(url, {'type': 'memberships'})
        assert response.json() == {
            'code': 0,
            'roles': [{
                'login': 'aragorn',
                'path': manager_path
            }],
            'next-url': url+'?type=memberships&since=%d' % membership_id
        }
        response = client.json.get(url, {'type': 'memberships', 'since': membership_id})
        assert response.json() == {
            'code': 0,
            'roles': [{
                'login': 'barlog',
                'path': admin_path
            }]
        }


def test_wrong_type(client):
    url = reverse('client-api:get-roles')
    response = client.json.get(url, {'type': 'wtf'})
    assert response.json() == {
        'code': 400,
        'fatal': 'Select a valid choice. wtf is not one of the available choices.'
    }


def test_over_limit(client):
    url = reverse('client-api:get-roles')
    response = client.json.get(url, {'type': 'superusers', 'since': 1000000})
    assert response.json() == {
        'code': 0,
        'roles': [],
        'next-url': '/client-api/get-roles/?type=memberships'
    }
    response = client.json.get(url, {'type': 'memberships', 'since': 1000000})
    assert response.json() == {
        'code': 0,
        'roles': [],
    }


def test_bad_since(client):
    url = reverse('client-api:get-roles')
    response = client.json.get(url, {'type': 'superusers', 'since': 'the first day of Creation'})
    assert response.json() == {
        'code': 400,
        'fatal': 'Enter a whole number.'
    }
