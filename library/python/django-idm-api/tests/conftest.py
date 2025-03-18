# coding: utf-8
from __future__ import unicode_literals

import weakref

import pytest
from django.test.client import Client
from django_idm_api.tests.utils import JsonClient, create_user


@pytest.fixture
def frodo():
    return create_user('frodo')


class JSONClient(Client):
    def __init__(self, *args, **kwargs):
        self.json = JsonClient(weakref.proxy(self))
        super(JSONClient, self).__init__(*args, **kwargs)


@pytest.fixture()
def client(request, settings):
    jsonclient = JSONClient()

    def login(username):  # Пароль не обязательный
        settings.YAUTH_TEST_USER = username

    jsonclient.login = login
    return jsonclient
