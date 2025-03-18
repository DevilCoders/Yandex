# coding: utf-8
from __future__ import unicode_literals

import itertools

from django.db.models import F
from django.utils.dateparse import parse_datetime
from ids.registry import registry
from sync_tools.datagenerators import DjangoGenerator

from django_abc_data.conf import settings
from django_abc_data.models import AbcService
from django_abc_data.tvm import get_client


class AbcServiceGenerator(DjangoGenerator):
    model = AbcService
    sync_fields = ('external_id',)
    diff_fields = ('name', 'name_en', 'state', 'parent', 'slug', 'owner_login',
                   'created_at', 'modified_at', 'path', 'readonly_state')
    query_fields = ('id', 'name', 'state', 'parent.id', 'slug', 'owner.login', 'created_at',
                    'modified_at', 'path', 'readonly_state')

    def __init__(self, external_ids=None, **kwargs):
        super(AbcServiceGenerator, self).__init__(**kwargs)

        self.external_ids = external_ids
        if self.external_ids is not None:
            self.queryset = self.get_queryset().filter(external_id__in=self.external_ids)

    def get_external_objects(self):
        oauth_token = settings.ABC_DATA_IDS_OAUTH_TOKEN
        page_size = settings.ABC_DATA_IDS_PAGE_SIZE
        timeout = settings.ABC_DATA_IDS_TIMEOUT
        user_agent = settings.ABC_DATA_IDS_USER_AGENT
        api_version = settings.ABC_DATA_API_VERSION
        tvm_client_id = settings.ABC_DATA_TVM_CLIENT_ID

        if oauth_token is None and tvm_client_id is None:
            raise AssertionError('ABC_DATA_IDS_OAUTH_TOKEN or  ABC_DATA_TVM_CLIENT_ID should be specified')

        kwargs = {
            'api_version': api_version,
            'user_agent': user_agent,
            'timeout': timeout,
        }

        if tvm_client_id is not None:
            tvm_service_ticket = get_client().get_service_ticket_for("abc")
            kwargs['service_ticket'] = tvm_service_ticket
        elif oauth_token is not None:
            kwargs['oauth_token'] = oauth_token
        repo = registry.get_repository(
            'abc', 'service', **kwargs,
        )

        for service in repo.getiter({'format': 'json',
                                     'page_size': page_size,
                                     'fields': ','.join(AbcServiceGenerator.query_fields),
                                     }):
            if self.external_ids is not None and service['id'] not in self.external_ids:
                continue

            owner = service.get('owner')
            owner_login = owner.get('login') if owner is not None else None

            parent = service['parent']
            parent_external_id = parent['id'] if parent else None
            yield {
                'external_id': service['id'],
                'name': service['name']['ru'],
                'name_en': service['name']['en'],
                'state': service['state'],
                'parent': parent_external_id,
                'slug': service['slug'],
                'owner_login': owner_login,
                'created_at': parse_datetime(service['created_at']),
                'modified_at': parse_datetime(service['modified_at']),
                'path': service['path'],
                'readonly_state': service['readonly_state'],
            }

    def get_internal_objects(self):
        with_parents = (
            self.get_queryset()
                .filter(parent__isnull=False)
                .values(*(field for field in self.value_fields if field != 'parent'))
                .annotate(parent=F('parent__external_id'))
        )
        without_parents = (
            self.get_queryset()
                .filter(parent__isnull=True)
                .values(*self.value_fields)
        )
        return itertools.chain(without_parents, with_parents)
