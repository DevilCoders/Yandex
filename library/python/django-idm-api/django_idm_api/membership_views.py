# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import json
import logging

from django.views.decorators.http import require_POST, require_GET
from django.views.decorators.csrf import csrf_exempt

from django_idm_api import validation
from django_idm_api.exceptions import UnsupportedApi
from django_idm_api.utils import get_hooks, as_json, token_auth

JSONDecodeError = ValueError


log = logging.getLogger(__name__)

__all__ = (
    'add_membership',
    'remove_membership',
    'get_memberships',
)


def _add_membership(data, **kwargs):
    hooks = get_hooks(**kwargs)
    form = validation.AddMembershipForm(data)
    if not form.is_valid():
        form.raise_first()
    return hooks.add_membership(*form.get_clean_data())


def _remove_membership(data, **kwargs):
    hooks = get_hooks(**kwargs)
    form = validation.RemoveMembershipForm(data)
    if not form.is_valid():
        form.raise_first()
    return hooks.remove_membership(*form.get_clean_data())


@as_json
@require_GET
@token_auth
def get_memberships(request, **kwargs):
    hooks = get_hooks(**kwargs)
    try:
        memberships = hooks.get_memberships(request)
    except NotImplementedError:
        raise UnsupportedApi('Unsupported API method')
    return memberships


@as_json
@require_POST
@token_auth
@csrf_exempt
def add_membership(request, **kwargs):
    return _add_membership(request.POST, **kwargs)


@as_json
@require_POST
@token_auth
@csrf_exempt
def remove_membership(request, **kwargs):
    return _remove_membership(request.POST, **kwargs)


@as_json
@require_POST
@token_auth
@csrf_exempt
def add_batch_memberships(request, **kwargs):
    memberships = json.loads(request.POST.get('data'))
    statuses = []
    for membership in memberships:
        try:
            _add_membership(membership, **kwargs)
        except Exception as err:
            membership['error'] = '%s' % err
            statuses.append(membership)
    if statuses:
        return {'code': 207, 'multi_status': statuses}
    return {'code': 0}


@as_json
@require_POST
@token_auth
@csrf_exempt
def remove_batch_memberships(request, **kwargs):
    memberships = json.loads(request.POST.get('data'))
    statuses = []
    for membership in memberships:
        try:
            _remove_membership(membership, **kwargs)
        except Exception as err:
            membership['error'] = '%s' % err
            statuses.append(membership)
    if statuses:
        return {'code': 207, 'multi_status': statuses}
    return {'code': 0}
