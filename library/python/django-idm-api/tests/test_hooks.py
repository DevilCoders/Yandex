# coding: utf-8
from __future__ import unicode_literals

import pytest
try:
    from django.urls import reverse
except ImportError:
    from django.core.urlresolvers import reverse
from django.test.utils import override_settings
from django_idm_api.hooks import AuthHooks
from django_idm_api.tests.utils import create_group, create_user

pytestmark = pytest.mark.django_db


def test_info_customization(client):
    """Проверим работу с переопределённым методом info() hook-ов"""
    url = reverse('client-api:info')

    class CustomHooks(AuthHooks):
        def info(self):
            data = super(CustomHooks, self).info()
            data['roles']['values']['project-manager'] = 'Менеджер проекта'
            return data

    with override_settings(ROLES_HOOKS=CustomHooks):
        response = client.json.get(url)
        assert response.status_code == 200
        assert response.json() == {
            'code': 0,
            'roles': {
                'slug': 'role',
                'name': 'роль',
                'values': {
                    'superuser': 'суперпользователь',
                    'project-manager': 'Менеджер проекта',
                },
            }
        }

        managers = create_group('Менеджеры')
        response = client.json.get(url)
        assert response.status_code == 200
        assert response.json() == {
            'code': 0,
            'roles': {
                'slug': 'role',
                'name': 'роль',
                'values': {
                    'superuser': 'суперпользователь',
                    'group-%d' % managers.pk: 'менеджеры',
                    'project-manager': 'Менеджер проекта',
                }
            }
        }


def test_add_remove_role_hooks(client):
    """Проверим работу с полностью переопределёнными hook-ами"""
    class CustomHooks(AuthHooks):
        is_granted = None

        def info(self):
            data = super(CustomHooks, self).info()
            data['roles']['values']['project-manager'] = 'Менеджер проекта'
            return data

        def add_role(self, user, role, fields, subject_type):
            CustomHooks.is_granted = True
            return self._successful_result

        def remove_role(self, user, role, data, is_fired, subject_type):
            CustomHooks.is_granted = False
            return self._successful_result

    with override_settings(ROLES_HOOKS=CustomHooks):
        url = reverse('client-api:add-role')
        response = client.json.post(url, {
            'login': 'frodo',
            'role': {
                'role': 'project-manager',
            }
        })
        assert response.status_code == 200
        assert response.json() == {'code': 0}
        assert CustomHooks.is_granted is True

        url = reverse('client-api:remove-role')
        client.json.post(url, {
            'login': 'frodo',
            'role': {
                'role': 'project-manager',
            }
        })
        assert response.status_code == 200
        assert response.json() == {'code': 0}
        assert CustomHooks.is_granted is False


def test_override_get_user_roles(client):
    """Проверим работу c hook-ами, где переопределён только метод get_user_roles"""

    manager = create_group('Менеджер')
    admin = create_group('Админ')

    aragorn = create_user('aragorn')
    aragorn.groups.add(manager)
    barlog = create_user('barlog')
    barlog.groups.add(admin)
    sam = create_user('sam')
    sam.is_superuser = True
    sam.save()

    class CustomHooks(AuthHooks):
        def get_user_roles_impl(self, login, **kwargs):
            result = super(CustomHooks, self).get_user_roles_impl(login)
            result.append({'role': 'wow'})
            return result

    url = reverse('client-api:get-all-roles')
    with override_settings(ROLES_HOOKS=CustomHooks):
        response = client.json.get(url)
    assert response.json() == {
        'code': 0,
        'users': [
            {
                'login': 'aragorn',
                'roles': [{'role': 'group-%d' % manager.id}, {'role': 'wow'}]
            },
            {
                'login': 'barlog',
                'roles': [{'role': 'group-%d' % admin.pk}, {'role': 'wow'}]
            },
            {
                'login': 'sam',
                'roles': [{'role': 'superuser'}, {'role': 'wow'}]
            }
        ]
    }
