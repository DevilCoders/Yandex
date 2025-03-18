# -*- coding:utf-8 -*-

from django.shortcuts import redirect
from django.http import HttpResponse
from django.db import DatabaseError
from django.core.exceptions import PermissionDenied

from functools import wraps

from core.actions import ang
from core.hard.loghandlers import SteamLogger
from core.models import User


def permission_required(groups):
    def decorator(view_func):
        @wraps(view_func)
        def _wrapped_view(request, *args, **kwargs):
            try:
                user = request.yauser.core_user
            except AttributeError:
                raise PermissionDenied
            if user and user.status != User.Status.WAIT_APPROVE:
                if user.role in groups:  # TODO: or user.is_superuser
                    if user.status == User.Status.ACTIVE:
                        return view_func(request, *args, **kwargs)
                    else:  # user.status == User.Satus.ANG
                        if ang.get_user_status(request):
                            return view_func(request, *args, **kwargs)
                raise PermissionDenied
            else:
                return redirect('core:request_permission')
        return _wrapped_view
    return decorator


def retry(attempts=3, is_view=True):
    def decorator(func):
        def wrapper(*args, **kwargs):
            cur_attempt = 0
            while True:
                cur_attempt += 1
                try:
                    return func(*args, **kwargs)
                except DatabaseError as exc:
                    if cur_attempt > attempts:
                        if is_view:
                            SteamLogger.error(
                                'DatabaseError rate exceeded retry limit with error %(error)s',
                                type='DATABASE_ERROR', error=exc
                            )
                            return HttpResponse('Service Temporarily Unavailable', status=503)
                        else:
                            raise
                    pass
        return wrapper
    return decorator
