# coding: utf-8

from __future__ import unicode_literals

import os
from django.test.utils import override_settings
from django.test import RequestFactory
from django.conf import settings
from django.contrib.auth import authenticate


FACTORY = RequestFactory(
    SERVER_NAME=settings.TEST_HOST,
    REMOTE_ADDR=settings.TEST_IP,
    SERVER_PORT=443,
    HTTP_AUTHORIZATION='OAuth %s' % settings.TEST_YANDEX_OAUTH_TOKEN,
)
TEST_LOGIN = 'volozh'
TEST_UID = '1120000000000529'


@override_settings(
    AUTHENTICATION_BACKENDS=[
        'django_yauth.authentication_mechanisms.dev.UserFromHttpHeaderAuthBackend',
    ],
)
def test_user_from_http_header():
    request = FACTORY.get(
        '',
        HTTP_DEBUG_LOGIN=TEST_LOGIN,
        HTTP_DEBUG_UID=TEST_UID,
    )

    yauser = authenticate(request=request)

    assert yauser is not None
    assert yauser.login == TEST_LOGIN
    assert yauser.uid == TEST_UID


@override_settings(
    AUTHENTICATION_BACKENDS=[
        'django_yauth.authentication_mechanisms.dev.UserFromCookieAuthBackend',
    ],
)
def test_user_from_cookies():
    request = FACTORY.get('')
    request.COOKIES = {
        'yandex_login': TEST_LOGIN,
        'yandex_uid': TEST_UID,
    }

    yauser = authenticate(request=request)

    assert yauser is not None
    assert yauser.login == TEST_LOGIN
    assert yauser.uid == TEST_UID


@override_settings(
    AUTHENTICATION_BACKENDS=[
        'django_yauth.authentication_mechanisms.dev.UserFromSettingsAuthBackend',
    ],
    YAUTH_DEV_USER_LOGIN=TEST_LOGIN,
    YAUTH_DEV_USER_UID=TEST_UID,
)
def test_user_from_settings():
    request = FACTORY.get('')

    yauser = authenticate(request=request)

    assert yauser is not None
    assert yauser.login == TEST_LOGIN
    assert yauser.uid == TEST_UID


@override_settings(
    AUTHENTICATION_BACKENDS=[
        'django_yauth.authentication_mechanisms.dev.UserFromOsEnvAuthBackend',
    ],
)
def test_user_from_env():
    request = FACTORY.get('')
    os.environ['USER'] = TEST_LOGIN
    os.environ['UID'] = TEST_UID

    yauser = authenticate(request=request)

    assert yauser is not None
    assert yauser.login == TEST_LOGIN
    assert yauser.uid == TEST_UID
