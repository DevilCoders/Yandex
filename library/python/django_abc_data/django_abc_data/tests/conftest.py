# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import pytest


@pytest.fixture
def value_fields():
    return ('external_id', 'name', 'name_en', 'state', 'slug', 'owner_login',
            'created_at', 'modified_at', 'path')


@pytest.fixture
def abc_services():
    return [
        {
            'id': 2,
            'created_at': '2016-12-31T23:59:59+00:00',
            'modified_at': '2017-04-02T07:32:43+00:00',
            'slug': 'test_service',
            'name': {
                'ru': 'Тестовый сервис',
                'en': 'Test service'
            },
            'owner': {
                'id': '7',
                'login': 'testuser',
                'first_name': 'Джеймс',
                'last_name': 'Бонд',
                'uid': '1111111111111111'
            },
            'parent': {
                'id': 1,
                'slug': 'parent_service',
                'name': {
                    'ru': 'Родительский сервис',
                    'en': 'Parent service'
                },
                'parent': None
            },
            'path': '/parent_service/test_service/',
            'readonly_state': None,
            'state': 'develop',
        },
        {
            'id': 1,
            'created_at': '2015-12-31T23:59:59+00:00',
            'modified_at': '2017-04-02T07:32:43+00:00',
            'slug': 'parent_service',
            'name': {
                    'ru': 'Родительский сервис',
                    'en': 'Parent service'
                },
            'owner': {
                'id': '66',
                'login': 'anotheruser',
                'first_name': 'Иван',
                'last_name': 'Иванов',
                'uid': '1111111111111112'
            },
            'parent': None,
            'path': '/parent_service/',
            'readonly_state': None,
            'state': 'develop',
        },
        {
            'id': 3,
            'created_at': '2016-12-31T23:59:58+00:00',
            'modified_at': '2017-06-02T07:32:43+00:00',
            'slug': 'another_service',
            'name': {
                'ru': 'Другой сервис',
                'en': 'Another service'
            },
            'owner': {
                'id': '1',
                'login': 'neo',
                'first_name': 'Томас',
                'last_name': 'Андерсон',
                'uid': '1111111111111111'
            },
            'parent': {
                'id': 1,
                'slug': 'parent_service',
                'name': {
                    'ru': 'Родительский сервис',
                    'en': 'Parent service'
                },
                'parent': None
            },
            'path': '/parent_service/another_service/',
            'readonly_state': None,
            'state': 'deleted',
        },
    ]


@pytest.fixture(autouse=True)
def no_external_calls(monkeypatch):
    """Для тестов никуда по урлам не ходим, все локально"""
    def opener(patched):
        def wrapper(url, *args, **kwargs):
            raise Exception("You forget to mock %s locally, it tries to open %s" % (patched, url))
        return wrapper
    monkeypatch.setattr('requests.request', opener('requests.request'))

    def get_class(patched):
        def wrapper(*args, **kwargs):
            raise Exception("You forget to mock %s locally" % patched)
        return wrapper
    monkeypatch.setattr('urllib.URLopener', get_class('urllib.URLopener'))
    monkeypatch.setattr('httplib.HTTPConnection', get_class('httplib.HTTPConnection'))
