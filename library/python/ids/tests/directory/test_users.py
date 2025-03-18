# coding: utf-8
from __future__ import unicode_literals

import json
from requests import Response
from unittest import TestCase

from ids.registry import registry


def get_repository():
    return registry.get_repository('directory', 'user', token='faketoken', user_agent='test-ua')


def get_request_patch(pages_count):

    def request_patch(method, url, **params):
        if pages_count == 0:
            result = []
        else:
            result = [{
                'method': method,
                'url': url,
                'params': params,
            }]
        data = {
            'page': params['params'].get('page', 1),
            'pages': pages_count,
            'total': pages_count * 2,
            'per_page': 2,
            'result': result
        }

        response = Response()
        response._content = json.dumps(data)
        return response

    return request_patch


class DirectoryUsersTest(TestCase):

    @staticmethod
    def _get_users(pages_count, per_page=None):
        repo = get_repository()
        repo.connector.session.request = get_request_patch(pages_count)

        lookup = {
            'x_org_id': 1,
            'nickname': 'user1,user2',
        }
        if per_page is not None:
            lookup['per_page'] = per_page

        users = repo.getiter(lookup)
        return [user for user in users]

    def test_empty(self):
        users = self._get_users(pages_count=0)
        self.assertEqual(len(users), 0)

    def test_one(self):
        users = self._get_users(pages_count=1, per_page=2)
        expected = [
            {
                u'url': u'https://api-internal-test.directory.ws.yandex.net/users/',
                u'params': {
                    u'headers': {
                        u'X-Org-ID': '1',
                        u'Authorization': u'Token faketoken',
                        u'User-Agent': u'test-ua'
                    },
                    u'params': {
                        u'nickname': u'user1,user2',
                        u'per_page': 2
                    },
                    u'timeout': 2
                },
                u'method': u'GET'
            }
        ]
        self.assertEqual(users, expected)

    def test_two(self):
        users = self._get_users(pages_count=2)
        expected = [
            {
                u'url': u'https://api-internal-test.directory.ws.yandex.net/users/',
                u'params': {
                    u'headers': {
                        u'X-Org-ID': '1',
                        u'Authorization': u'Token faketoken',
                        u'User-Agent': u'test-ua'
                    },
                    u'params': {
                        u'nickname': u'user1,user2',
                        u'per_page': 1000
                    },
                    u'timeout': 2
                },
                u'method': u'GET'
            },
            {
                u'url': u'https://api-internal-test.directory.ws.yandex.net/users/',
                u'params': {
                    u'headers': {
                        u'Authorization': u'Token faketoken',
                        u'User-Agent': u'test-ua'
                    },
                    u'params': {
                        u'nickname': u'user1,user2',
                        u'per_page': 1000,
                        u'page': 2
                    },
                    u'timeout': 2
                },
                u'method': u'GET'
            }
        ]
        self.assertEqual(users, expected)
