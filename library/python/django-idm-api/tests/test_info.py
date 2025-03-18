# coding: utf-8
from __future__ import unicode_literals
import pytest
try:
    from django.urls import reverse
except ImportError:
    from django.core.urlresolvers import reverse
from django.test.utils import override_settings
from django_idm_api.tests.utils import create_group

pytestmark = pytest.mark.django_db


def test_get_info_shows_all_groups(client):
    url = reverse('client-api:info')
    expected = {
        'code': 0,
        'roles': {
            'slug': 'role',
            'name': 'роль',
            'values': {
                'superuser': 'суперпользователь'
            }
        }
    }
    assert client.json.get(url).json() == expected
    managers = create_group('Менеджеры')

    expected = {
        'code': 0,
        'roles': {
            'slug': 'role',
            'name': 'роль',
            'values': {
                'superuser': 'суперпользователь',
                'group-%s' % managers.pk: 'менеджеры',
            },
        }
    }

    assert client.json.get(url).json() == expected


def test_token_auth(client):
    url = reverse('client-api:info')
    token = 'mellon'

    with override_settings(ROLES_TOKENS=[token]):
        response = client.json.get(url)
        assert response.status_code == 200
        assert response.json() == {
            'code': 403,
            'fatal': 'Access denied, bad token.'
        }

        response = client.json.get(url, {'token': token})
        assert response.status_code == 200
        assert response.json() == {
            'code': 0,
            'roles': {
                'slug': 'role',
                'name': 'роль',
                'values': {
                    'superuser': 'суперпользователь',
                },
            }
        }
