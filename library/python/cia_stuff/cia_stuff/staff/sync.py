# coding: utf-8

from __future__ import unicode_literals

import logging
from itertools import chain

from django.conf import settings

from cia_stuff.staff import api

logger = logging.getLogger(__name__)


def prepare_request_for_load_group_membership_from_staff():
    groups_url = settings.STAFF_API_BASE_URL + 'groups'
    groups_api = api.ApiWrapper(
        groups_url, settings.STAFF_TOKEN, page_size=5000,
        fields=['id', 'department.heads.person.id', 'department.heads.role'],
        filter_args={'type': 'department', '_sort': 'id'},
    )

    groupmembership_url = settings.STAFF_API_BASE_URL + 'groupmembership'
    groupmembership_api = api.ApiWrapper(
        groupmembership_url, settings.STAFF_TOKEN, page_size=10000,
        fields=['person.id', 'group.id'],
        filter_args={
            '_sort': 'id',
            'group.type': 'department',
            'group.is_deleted': 'false',
        },
    )
    return groups_api, groupmembership_api


def generate_group_memberships(groups, memberships):
    members_by_group = {}
    main_group_by_id = {}
    for membership in memberships:
        group_id = membership['group']['id']
        person_id = membership['person']['id']
        group_members = members_by_group.setdefault(group_id, [])
        group_members.append(person_id)
        main_group_by_id[person_id] = group_id

    for g in groups:
        group_id = g['id']
        heads = g['department']['heads']
        chiefs = set(h['person']['id'] for h in heads if h['role'] == 'chief')
        deputies = set(h['person']['id'] for h in heads if h['role'] == 'deputy')
        members = members_by_group.get(group_id, [])
        all_members = set(chain(chiefs, deputies, members))
        for person_id in all_members:
            main_group = main_group_by_id.get(person_id)

            if main_group is None:
                logger.error('No main group for %d', person_id)

            yield {
                'group_id': group_id,
                'person_id': person_id,
                'is_chief': person_id in chiefs,
                'is_deputy': person_id in deputies,
                'is_primary': main_group_by_id.get(person_id) == group_id,
            }
