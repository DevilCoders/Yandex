# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import importlib
import json
import logging
from functools import wraps

from django.conf import settings
from django.http import HttpResponse
try:
    from django.utils.encoding import force_text
except ImportError:
    from django.utils.encoding import force_str as force_text
try:
    from django.utils.six import string_types
except ImportError:
    string_types = (str, )
from django_idm_api.exceptions import BadRequest, AccessDenied

log = logging.getLogger(__name__)


def get_hooks(**kwargs):
    """Возвращает объект, реализующий хуки."""
    hooks_cls = getattr(settings, 'ROLES_HOOKS', 'django_idm_api.hooks.AuthHooks')
    if isinstance(hooks_cls, string_types):
        module_name, class_name = hooks_cls.rsplit('.', 1)
        module = importlib.import_module(module_name)
        hooks_cls = getattr(module, class_name)
    return hooks_cls(**kwargs)


def as_json(func):
    """Декоратор, сериализующий результат функции в JSON и возвращающий HTTPResponse.
    """
    @wraps(func)
    def wrapper(*args, **kwargs):
        try:
            result = func(*args, **kwargs)
        except BadRequest as e:
            message = force_text(e)
            log.error('wrong params, during call to %s: %s', func.__name__, message)
            result = {
                'code': 400,
                'fatal': message
            }
        except AccessDenied as e:
            message = force_text(e)
            log.error('access denied, during call to %s: %s', func.__name__, message)
            result = {
                'code': 403,
                'fatal': message
            }
        except Exception as e:
            log.exception('during call to: %s', func.__name__)
            result = {
                'code': 500,
                'error': force_text(e)
            }
        if not isinstance(result, HttpResponse):
            result = HttpResponse(
                json.dumps(result, ensure_ascii=False),
                content_type='application/json'
            )
        return result
    return wrapper


def token_auth(func):
    """Декоратор, который проверяет, что параметр token входит в список допустимых токенов.

    Проверка осуществляется только в том случае, если задана настройка ROLES_TOKENS.
    Токен может быть указан как в URL, так и в теле POST запроса.
    """
    @wraps(func)
    def wrapper(request, *args, **kwargs):
        allowed_tokens = getattr(settings, 'ROLES_TOKENS', None)
        token = request.GET.get('token')
        if token is None:
            token = request.POST.get('token')

        if allowed_tokens is not None and token not in allowed_tokens:
            raise AccessDenied('Access denied, bad token.')
        return func(request, *args, **kwargs)
    return wrapper


def is_group_request(request):
    system_is_group_aware = getattr(settings, 'ROLES_SYSTEM_IS_GROUP_AWARE', False)
    return request.method == 'POST' and 'group' in request.POST and system_is_group_aware


def is_b2b():
    env_name = getattr(settings, 'IDM_ENVIRONMENT_TYPE', None)
    return not (env_name is None or env_name == 'intranet')


def with_environment_options(intranet, b2b):
    return b2b if is_b2b() else intranet
