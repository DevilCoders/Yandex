from django.contrib.auth.decorators import user_passes_test
from django.contrib.auth import REDIRECT_FIELD_NAME


try:
    from rest_framework.permissions import BasePermission
except ImportError:
    class BasePermission(object):
        def has_permission(self, request, view):
            raise RuntimeError('Trying to use rest_framework classes, but contrib/python/djangorestframework not in PEERDIR')


def superuser_required(view_func=None, redirect_field_name=REDIRECT_FIELD_NAME, login_url='admin:login'):
    """
    Django decorator for views
    By default is_staff always true for staff accounts. And IDM generate admin role Superuser.
    """
    actual_decorator = user_passes_test(
        lambda u: u.is_active and u.is_staff and u.is_superuser,
        login_url=login_url,
        redirect_field_name=redirect_field_name
    )
    if view_func:
        return actual_decorator(view_func)
    return actual_decorator


class IsSuperUser(BasePermission):
    """
    Rest Framework decorator for checking SuperUser
    By default is_staff always true for staff accounts. And IDM generate admin role Superuser.
    """
    def has_permission(self, request, view):
        return bool(request.user and request.user.is_staff and request.user.is_superuser)
