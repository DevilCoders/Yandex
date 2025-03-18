# coding: utf-8
from __future__ import unicode_literals
import pytest
try:
    from django.urls import reverse
except ImportError:
    from django.core.urlresolvers import reverse
from django.test.utils import override_settings
from django_idm_api.hooks import BaseHooks, B2BHooksMixin
from django_idm_api.tests.utils import create_group, refresh
from django_idm_api.compat import get_user_model

pytestmark = pytest.mark.django_db


def test_add_superuser_role(client, frodo):
    url = reverse('client-api:add-role')

    assert frodo.is_superuser is False
    assert frodo.is_staff is False
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
    assert frodo.is_superuser is True
    assert frodo.is_staff is True


def test_add_group_as_role(client, frodo):
    url = reverse('client-api:add-role')

    group = create_group('Менеджеры')
    assert frodo.groups.count() == 0
    assert frodo.is_staff is False
    response = client.json.post(url, {
        'login': 'frodo',
        'role': {
            'role': 'group-%d' % group.pk
        }
    })
    assert response.status_code == 200
    assert response.json() == {
        'code': 0
    }

    frodo = refresh(frodo)
    assert frodo.groups.count() == 1
    assert frodo.groups.get().name == 'Менеджеры'
    assert frodo.is_staff is True


def test_add_role_bad_request(client, frodo):
    url = reverse('client-api:add-role')
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

    # null role request
    response = client.json.post(url, {
        'login': 'unknown-login',
        'role': {
            'role': None
        }
    })
    assert response.status_code == 200
    assert response.json() == {
        'code': 400,
        'fatal': 'Role is not not provided'
    }


def test_add_role_creates_user(client):
    url = reverse('client-api:add-role')

    group = create_group('Менеджеры')

    assert get_user_model().objects.filter(username='legolas').count() == 0

    response = client.json.post(url, {
        'login': 'legolas',
        'role': {
            'role': 'group-%d' % group.pk
        }
    })
    assert response.status_code == 200
    assert response.json() == {'code': 0}
    user = get_user_model().objects.get(username='legolas')
    assert user.groups.count() == 1
    assert user.groups.get() == group
    assert user.username == 'legolas'
    assert user.is_staff is True
    assert user.is_active is True
    assert user.is_superuser is False


def test_group_add_role_is_not_recognized_if_group_is_unaware(client):
    """Проверим, что по умолчанию система ничего не знает о групповых ролях и не понимает, чего от неё хотят"""

    url = reverse('client-api:add-role')
    response = client.json.post(url, {
        'group': 13,
        'role': {
            'role': 'superuser'
        }
    })
    assert response.status_code == 200
    assert response.json() == {
        'code': 400,
        'fatal': 'Field "login" is required.'
    }


def test_add_group_role_if_system_is_aware(client):
    """Если включена соответствующая настройка, то система сама следит за составом групп.
    Мы (пока?) не предоставляем готового механизма для отслеживания состава групп, так что хуки тоже
    будут переопределены"""

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
        assert CustomHooks.role_is_added is True
        assert CustomHooks.group == 13
        assert CustomHooks.role == {'role': 'superuser'}
        assert CustomHooks.fields == {'token': 'mellon'}


def test_add_tvm_app_role(client):
    url = reverse('client-api:add-role')

    class CustomHooks(BaseHooks):
        role_is_added = None

        def add_tvm_role_impl(self, login, role, fields, **kwargs):
            CustomHooks.role_is_added = True
            CustomHooks.login = login
            CustomHooks.role = role
            CustomHooks.fields = fields
            return self._successful_result

    with override_settings(ROLES_HOOKS=CustomHooks):
        response = client.json.post(url, {
            'login': 'tvm_app_123',
            'role': {
                'role': 'superuser'
            },
            'subject_type': 'tvm_app'
        })
        assert response.status_code == 200
        assert response.json() == {'code': 0}
        assert CustomHooks.role_is_added is True
        assert CustomHooks.login == 'tvm_app_123'
        assert CustomHooks.role == {'role': 'superuser'}


def test_add_tvm_app_group_role(client):
    url = reverse('client-api:add-role')

    class CustomHooks(BaseHooks):
        role_is_added = None

        def add_tvm_group_role_impl(self, group, role, fields, **kwargs):
            CustomHooks.role_is_added = True
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
        assert CustomHooks.role_is_added is True
        assert CustomHooks.group == 13
        assert CustomHooks.role == {'role': 'superuser'}


def test_add_role_in_b2b_error(client):
    url = reverse('client-api:add-role')

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


def test_add_user_role_in_b2b(client):
    url = reverse('client-api:add-role')

    class CustomHooks(B2BHooksMixin, BaseHooks):
        role_is_added = None

        def add_role_impl(self, uid, role, fields, org_id, **kwargs):
            CustomHooks.role_is_added = True
            CustomHooks.uid = uid
            CustomHooks.role = role
            CustomHooks.fields = fields
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
        assert CustomHooks.role_is_added is True
        assert CustomHooks.uid == 'uid_123'
        assert CustomHooks.role == {'role': 'superuser'}
        assert CustomHooks.org_id == 123


def test_add_group_role_in_b2b(client):
    url = reverse('client-api:add-role')

    class CustomHooks(B2BHooksMixin, BaseHooks):
        role_is_added = None

        def add_group_role_impl(self, group, role, fields, org_id, **kwargs):
            CustomHooks.role_is_added = True
            CustomHooks.group = group
            CustomHooks.role = role
            CustomHooks.fields = fields
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
        assert CustomHooks.role_is_added is True
        assert CustomHooks.group == 321
        assert CustomHooks.role == {'role': 'superuser'}
        assert CustomHooks.org_id == 123
