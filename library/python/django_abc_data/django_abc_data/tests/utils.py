# coding: utf-8
from __future__ import unicode_literals

from contextlib import contextmanager

from mock import patch

from django.utils import timezone
from django.utils.dateparse import parse_datetime

from django_abc_data.models import AbcService
from django_abc_data.tests.conftest import abc_services, value_fields


def refresh(obj):
    return type(obj).objects.get(pk=obj.pk)


@contextmanager
def patch_ids_repo(services=None):
    with patch('ids.services.abc.repositories.service.ServiceRepository.getiter') as getiter:
        if services is None:
            services = abc_services()
        getiter.return_value = services
        yield getiter


def create_service(external_id, **kwargs):
    params = dict(
        external_id=external_id,
        name='',
        name_en='',
        state='',
        slug='',
        owner_login=None,
        created_at=timezone.now(),
        modified_at=timezone.now(),
        path='',
    )
    params.update(kwargs)
    return AbcService.objects.create(**params)


def assert_services(internal, external):
    diff = {}
    for key in value_fields():
        if key == 'external_id':
            int_value = internal.external_id
            ext_value = external['id']
        elif key == 'parent':
            int_value = internal.parent.external_id
            ext_value = external['parent']['id'] if external['parent'] else None
        elif key == 'owner_login':
            int_value = internal.owner_login
            ext_value = external['owner']['login'] if external['owner'] else None
        elif key == 'name':
            int_value = internal.name
            ext_value = external['name']['ru']
        elif key == 'name_en':
            int_value = internal.name_en
            ext_value = external['name']['en']
        elif key in ('created_at', 'modified_at'):
            int_value = getattr(internal, key, None)
            ext_value = parse_datetime(external[key])
        else:
            int_value = getattr(internal, key, None)
            ext_value = external[key]

        if int_value != ext_value:
            diff[key] = (int_value, ext_value)

    if diff:
        diff_list = [('%s: (int=%s, ext=%s)' % (key, value[0], value[1])) for (key, value) in diff.items()]
        diff_message = ', '.join(diff_list)
        raise AssertionError('Objects differ: {%s}' % diff_list)

