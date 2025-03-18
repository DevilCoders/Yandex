# coding: utf-8
from __future__ import unicode_literals

import pytest
try:
    from django.urls import reverse
except ImportError:
    from django.core.urlresolvers import reverse
from django.test.utils import override_settings
from django_idm_api.hooks import BaseHooks, B2BHooksMixin
from django_idm_api.tests.utils import refresh, create_group

pytestmark = pytest.mark.django_db


def test_deprive_superuser_role(client, frodo):
    url = reverse('client-api:remove-role')

    frodo.is_superuser = True
    frodo.is_staff = True
    frodo.save()

    response = client.json.post(url, {
        'login': 'frodo',
        'role': {
            'role': 'superuser'
        }
    })
    assert response.status_code == 200
    assert response.json() == {
        'code': 0
    }

    frodo = refresh(frodo)
    assert frodo.is_superuser is False
    assert frodo.is_staff is False


def test_deprive_usual_role(client, frodo):
    url = reverse('client-api:remove-role')

    group = create_group('Менеджеры')
    frodo.groups.add(group)
    frodo.is_staff = True
    frodo.save()

    response = client.json.post(url, {
        'login': 'frodo',
        'role': {
            'role': 'group-%d' % group.pk
        }
    })
    assert response.status_code == 200
    assert response.json() == {'code': 0}
    frodo = refresh(frodo)
    assert frodo.groups.count() == 0
    assert frodo.is_staff is False


def test_deprive_unknown_role_returns_success(client):
    url = reverse('client-api:remove-role')
    response = client.json.post(url, {
        'login': 'unknown-login',
        'role': {
            'bad': 'role'
        }
    })
    assert response.status_code == 200
    assert response.json() == {'code': 0}


def test_deprive_role_bad_req(client):
    url = reverse('client-api:remove-role')

    # bad json request
    response = client.json.post(url, {
        'login': 'unknown-login',
        'role': 'gibberish'
    })
    assert response.status_code == 200
    assert response.json() == {
        'code': 400,
        'fatal': 'incorrect json data in field `role`: gibberish'
    }

    # null role request - возвращает ОК, т.к. мы не говорим, если такой роли не было
    response = client.json.post(url, {
        'login': 'unknown-login',
        'role': {
            'role': None
        }
    })
    assert response.status_code == 200
    assert response.json() == {'code': 0}


def test_is_staff_remains_if_other_roles(client, frodo):
    """Проверим, что is_staff остаётся, если у пользователя есть хотя бы одна роль"""
    url = reverse('client-api:remove-role')

    group = create_group('Менеджер')
    group2 = create_group('Админ')
    frodo.groups.add(group)
    frodo.groups.add(group2)
    frodo.is_staff = True
    frodo.save()

    response = client.json.post(url, {
        'login': 'frodo',
        'role': {
            'role': 'group-%d' % group.pk
        }
    })
    assert response.status_code == 200
    assert response.json() == {'code': 0}
    frodo = refresh(frodo)
    assert frodo.groups.count() == 1
    assert frodo.groups.get() == group2
    assert frodo.is_staff is True


def test_is_staff_remains_if_still_superuser(client, frodo):
    """Проверим, что is_staff остаётся, если пользователь - суперюзер"""
    url = reverse('client-api:remove-role')

    group = create_group('Менеджер')
    frodo.groups.add(group)
    frodo.is_superuser = True
    frodo.is_staff = True
    frodo.save()

    response = client.json.post(url, {
        'login': 'frodo',
        'role': {
            'role': 'group-%d' % group.pk
        }
    })
    assert response.status_code == 200
    assert response.json() == {'code': 0}

    frodo = refresh(frodo)
    assert frodo.groups.count() == 0
    assert frodo.is_superuser is True
    assert frodo.is_staff is True


def test_deprive_group_role_if_system_is_aware(client):
    """Если включена соответствующая настройка, то система сама следит за составом групп.
    Мы (пока?) не предоставляем готового механизма для отслеживания состава групп, так что хуки тоже
    будут переопределены"""

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
        response = client.json.post(url, {
            'group': 13,
            'role': {
                'role': 'superuser'
            },
            'fields': {
                'token': 'mellon'
            }
        })
        assert response.status_code == 200
        assert response.json() == {'code': 0}
        assert CustomHooks.group == 13
        assert CustomHooks.role == {'role': 'superuser'}
        assert CustomHooks.data == {'token': 'mellon'}
        assert CustomHooks.is_deleted is False


def test_deprive_role_for_fired_user(client):
    class CustomHooks(BaseHooks):
        is_fired = None

        def remove_role(self, user, role, data, is_fired, subject_type):
            CustomHooks.is_fired = is_fired
            return self._successful_result

    with override_settings(ROLES_HOOKS=CustomHooks):
        url = reverse('client-api:remove-role')
        response = client.json.post(url, {
            'login': 'frodo',
            'role': {
                'role': 'project-manager',
            },
            'fired': '1'
        })
        assert response.status_code == 200
        assert response.json() == {'code': 0}
        assert CustomHooks.is_fired is True


# TypeError: Cannot encode None for key 'fired' as POST data. Did you mean to pass an empty string or omit the value?
# The reason is that the default content type (multipart/form-data) doesn't support null values, only empty strings, hence the suggestion.
@pytest.mark.parametrize('fired', ['None', '', True, False, 'True', 'False', 'true', 'false', '0'])
def test_deprive_role_for_not_fired_user(client, fired):
    class CustomHooks(BaseHooks):
        is_fired = None

        def remove_role(self, user, role, data, is_fired, subject_type):
            CustomHooks.is_fired = is_fired
            return self._successful_result

    with override_settings(ROLES_HOOKS=CustomHooks):
        url = reverse('client-api:remove-role')
        response = client.json.post(url, {
            'login': 'frodo',
            'role': {
                'role': 'project-manager',
            },
            'fired': fired
        })
        assert response.status_code == 200
        assert response.json() == {'code': 0}
        assert CustomHooks.is_fired is False


def test_deprive_tvm_app_role(client):
    url = reverse('client-api:remove-role')

    class CustomHooks(BaseHooks):
        role_is_removed = None

        def remove_tvm_role_impl(self, login, role, fields, is_fired, **kwargs):
            CustomHooks.role_is_removed = True
            CustomHooks.login = login
            CustomHooks.role = role
            CustomHooks.fields = fields
            return self._successful_result

    with override_settings(ROLES_HOOKS=CustomHooks):
        response = client.json.post(url, {
            'login': 'frodo',
            'role': {
                'role': 'superuser'
            },
            'subject_type': 'tvm_app'
        })
        assert response.status_code == 200
        assert response.json() == {'code': 0}
        assert CustomHooks.role_is_removed is True
        assert CustomHooks.login == 'frodo'
        assert CustomHooks.role == {'role': 'superuser'}


def test_deprive_tvm_app_group_role(client):
    url = reverse('client-api:remove-role')

    class CustomHooks(BaseHooks):
        role_is_removed = None

        def remove_tvm_group_role_impl(self, group, role, fields, is_fired, **kwargs):
            CustomHooks.role_is_removed = True
            CustomHooks.group = group
            CustomHooks.role = role
            CustomHooks.fields = fields
            return self._successful_result

    with override_settings(ROLES_HOOKS=CustomHooks, ROLES_SYSTEM_IS_GROUP_AWARE=True):
        response = client.json.post(url, {
            'group': 13,
            'role': {
                'role': 'superuser'
            },
            'subject_type': 'tvm_app'
        })
        assert response.status_code == 200
        assert response.json() == {'code': 0}
        assert CustomHooks.role_is_removed is True
        assert CustomHooks.group == 13
        assert CustomHooks.role == {'role': 'superuser'}


def test_remove_role_in_b2b_error(client):
    url = reverse('client-api:remove-role')

    data = {
        'login': 'some_login',
        'role': {
            'role': 'superuser'
        },
    }

    class CustomHooks(B2BHooksMixin, BaseHooks):
        pass

    with override_settings(IDM_ENVIRONMENT_TYPE='other'):
        response = client.json.post(url, data)
        assert response.status_code == 200
        assert response.json() == {'code': 500, 'error': 'AuthHooks implemented only in intranet'}

        with override_settings(ROLES_HOOKS=CustomHooks):
            response = client.json.post(url, data)
            assert response.status_code == 200
            assert response.json() == {'code': 400, 'fatal': 'Field "org_id" is required.'}

            data['org_id'] = 123
            response = client.json.post(url, data)
            assert response.status_code == 200
            assert response.json() == {'code': 400, 'fatal': 'Field "uid" is required.'}


def test_remove_user_role_in_b2b(client):
    url = reverse('client-api:remove-role')

    class CustomHooks(B2BHooksMixin, BaseHooks):
        role_is_removed = None

        def remove_role_impl(self, uid, role, data, is_fired, org_id, **kwargs):
            CustomHooks.role_is_removed = True
            CustomHooks.uid = uid
            CustomHooks.role = role
            CustomHooks.data = data
            CustomHooks.org_id = org_id
            return self._successful_result

    with override_settings(ROLES_HOOKS=CustomHooks, IDM_ENVIRONMENT_TYPE='other'):
        response = client.json.post(url, {
            'uid': 'uid_123',
            'role': {
                'role': 'superuser'
            },
            'org_id': 123,
        })

        assert response.status_code == 200
        assert response.json() == {'code': 0}
        assert CustomHooks.role_is_removed is True
        assert CustomHooks.uid == 'uid_123'
        assert CustomHooks.role == {'role': 'superuser'}
        assert CustomHooks.org_id == 123


def test_remove_group_role_in_b2b(client):
    url = reverse('client-api:remove-role')

    class CustomHooks(B2BHooksMixin, BaseHooks):
        role_is_removed = None

        def remove_group_role_impl(self, group, role, data, is_deleted, org_id, **kwargs):
            CustomHooks.role_is_removed = True
            CustomHooks.group = group
            CustomHooks.role = role
            CustomHooks.data = data
            CustomHooks.org_id = org_id
            return self._successful_result

    with override_settings(ROLES_HOOKS=CustomHooks, IDM_ENVIRONMENT_TYPE='other', ROLES_SYSTEM_IS_GROUP_AWARE=True):
        response = client.json.post(url, {
            'group': 321,
            'role': {
                'role': 'superuser'
            },
            'org_id': 123,
        })

        assert response.status_code == 200
        assert response.json() == {'code': 0}
        assert CustomHooks.role_is_removed is True
        assert CustomHooks.group == 321
        assert CustomHooks.role == {'role': 'superuser'}
        assert CustomHooks.org_id == 123
