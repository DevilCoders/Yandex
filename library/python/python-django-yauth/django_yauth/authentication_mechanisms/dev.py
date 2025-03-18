# coding: utf-8

from __future__ import unicode_literals

import os

from django.conf import settings
from django_yauth.user import YandexUser


class DevAuthBackendBase(object):

    def authenticate(self, request):
        login = self.fetch_login(request)
        uid = self.fetch_uid(request)
        if login is None and uid is None:
            return
        return YandexUser(
            uid=uid,
            fields={
                'login': login,
                'language': 'ru'
            }
        )

    def fetch_login(self, request):
        return None

    def fetch_uid(self, request):
        return None


class UserFromHttpHeaderAuthBackend(DevAuthBackendBase):

    def fetch_login(self, request):
        return request.META.get('HTTP_DEBUG_LOGIN')

    def fetch_uid(self, request):
        return request.META.get('HTTP_DEBUG_UID')


class UserFromCookieAuthBackend(DevAuthBackendBase):
    def fetch_login(self, request):
        return request.COOKIES.get('yandex_login')

    def fetch_uid(self, request):
        return request.COOKIES.get('yandex_uid')


class UserFromSettingsAuthBackend(DevAuthBackendBase):

    def fetch_login(self, request):
        return getattr(settings, 'YAUTH_DEV_USER_LOGIN', None)

    def fetch_uid(self, request):
        return getattr(settings, 'YAUTH_DEV_USER_UID', None)


class UserFromOsEnvAuthBackend(DevAuthBackendBase):

    def fetch_login(self, request):
        return os.getenv('USER')

    def fetch_uid(self, request):
        return os.getenv('UID')
