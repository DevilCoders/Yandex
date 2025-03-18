# coding: utf-8
from __future__ import unicode_literals

import pytest
try:
    from django.urls import reverse
except ImportError:
    from django.core.urlresolvers import reverse
from django_idm_api.tests.utils import create_group, create_user

pytestmark = pytest.mark.django_db


def test_get_all_roles(client):
    manager = create_group('Менеджер')
    admin = create_group('Админ')

    aragorn = create_user('aragorn')
    aragorn.groups.add(manager)
    barlog = create_user('barlog')
    barlog.groups.add(admin)
    sam = create_user('sam')
    sam.is_superuser = True
    sam.save()
    # у Гимли нет ролей и его не должно быть в выдаче
    create_user('gimli')

    url = reverse('client-api:get-all-roles')
    response = client.json.get(url)
    assert response.json() == {
        'code': 0,
        'users': [
            {
                'login': 'aragorn',
                'roles': [{'role': 'group-%d' % manager.id}]
            },
            {
                'login': 'barlog',
                'roles': [{'role': 'group-%d' % admin.pk}]
            },
            {
                'login': 'sam',
                'roles': [{'role': 'superuser'}]
            }
        ]
    }
