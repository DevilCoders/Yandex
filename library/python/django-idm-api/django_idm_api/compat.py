from __future__ import unicode_literals

try:
    from django.contrib.auth import get_user_model
except ImportError:
    # Pre- django-1.5
    from django.contrib.auth.models import User
    get_user_model = lambda: User

try:
    # Django 4 and Django 3
    from django.urls import re_path as url, include
except ImportError:
    try:
        # fallback to Django < 3
        from django.conf.urls import url, include
    except ImportError:
        # fallback to Django < 1.6
        from django.conf.urls.defaults import url, include


__all__ = ('get_user_model', 'url', 'include')
