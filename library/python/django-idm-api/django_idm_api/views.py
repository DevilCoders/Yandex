# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging

from django.views.decorators.http import require_POST, require_GET
from django.views.decorators.csrf import csrf_exempt

from django_idm_api import validation
from django_idm_api.exceptions import UnsupportedApi
from django_idm_api.utils import get_hooks, as_json, token_auth, is_group_request

JSONDecodeError = ValueError


log = logging.getLogger(__name__)

__all__ = ('info', 'add_role', 'remove_role', 'get_all_roles', 'get_roles')


@as_json
@require_GET
@token_auth
def info(request, **kwargs):
    """Формируем дерево ролей, но отправляем только первые N ролей на каждом уровне, для остальных проставляем
    'next-url'. Если запрос уже включает 'next-url', отправляем следующие N ролей в соответствии в запросом.
    Сортируем каждый уровень 'values' по ключам и включаем в итоговое дерево только первые N."""

    try:
        hooks = get_hooks(**kwargs)
        tree = hooks.info()
    except Exception:
        log.exception('during "/info" with request: %s', request)
        raise

    if tree['code'] != 0:
        return tree

    if 'code' not in tree:
        tree['code'] = 0

    return tree


@as_json
@require_POST
@token_auth
@csrf_exempt
def add_role(request, **kwargs):
    hooks = get_hooks(**kwargs)
    if is_group_request(request):
        form = validation.AddGroupRoleForm(request.POST)
        method = hooks.add_group_role
    else:
        form = validation.AddUserRoleForm(request.POST)
        method = hooks.add_role
    if not form.is_valid():
        form.raise_first()

    return method(*form.get_clean_data())


@as_json
@require_POST
@token_auth
@csrf_exempt
def remove_role(request, **kwargs):
    hooks = get_hooks(**kwargs)
    if is_group_request(request):
        form = validation.RemoveGroupRoleForm(request.POST)
        method = hooks.remove_group_role
    else:
        form = validation.RemoveUserRoleForm(request.POST)
        method = hooks.remove_role
    if not form.is_valid():
        form.raise_first()

    return method(*form.get_clean_data())


@as_json
@require_GET
@token_auth
def get_all_roles(request, **kwargs):
    hooks = get_hooks(**kwargs)
    return hooks.get_all_roles()


@as_json
@require_GET
@token_auth
def get_roles(request, **kwargs):
    hooks = get_hooks(**kwargs)
    try:
        roles = hooks.get_roles(request)
    except NotImplementedError:
        raise UnsupportedApi('Unsupported API method')
    return roles
