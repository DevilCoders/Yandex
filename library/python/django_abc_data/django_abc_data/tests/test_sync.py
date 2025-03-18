# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import copy
import pytest

from django.core.management import call_command

from django_abc_data.core import sync_services
from django_abc_data.models import AbcService
from django_abc_data.tests.utils import patch_ids_repo, create_service, assert_services, refresh

pytestmark = pytest.mark.django_db


def test_sync_add_or_remove_objects(abc_services):
    assert AbcService.objects.count() == 0

    new_service = create_service(4)

    with patch_ids_repo():
        sync_services()

    assert AbcService.objects.filter(external_id=new_service.external_id).count() == 1

    assert AbcService.objects.count() == 4
    for internal, external in zip(AbcService.objects.exclude(external_id=4).order_by('pk'), abc_services):
        assert_services(internal, external)

    AbcService.objects.all().delete()
    assert AbcService.objects.count() == 0

    new_service = create_service(4)

    with patch_ids_repo():
        sync_services(delete=True)

    assert AbcService.objects.filter(external_id=new_service.external_id).count() == 0

    assert AbcService.objects.count() == 3
    for internal, external in zip(AbcService.objects.order_by('pk'), abc_services):
        assert_services(internal, external)


def test_sync_update_objects(abc_services):
    with patch_ids_repo():
        sync_services()

    for internal, external in zip(AbcService.objects.order_by('pk'), abc_services):
        assert_services(internal, external)

    changed_service = AbcService.objects.get(external_id=1)
    changed_service.state = 'deleted'
    changed_service.save()

    with patch_ids_repo():
        sync_services()

    changed_service = refresh(changed_service)
    assert changed_service.state == 'develop'

    ext_services = copy.deepcopy(abc_services)
    for service in ext_services:
        if service['id'] == changed_service.external_id:
            service['owner']['login'] = 'cute-kitty'
    with patch_ids_repo(services=ext_services):
        sync_services()

    changed_service = refresh(changed_service)
    assert changed_service.owner_login == 'cute-kitty'
