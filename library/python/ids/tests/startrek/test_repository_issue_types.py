# -*- coding: utf-8 -*-

import unittest
import pytest
from copy import copy

import six

from ids.exceptions import OperationNotPermittedError
from ids.registry import registry


def _assertEqualIssueTypes(x, y):
    assert x['new_name'] == y['new_name'] and x['id'] == y['id']


class TestStartrekIssueTypesRepository(unittest.TestCase):
    def setUp(self):
        self.token = 'c24ce44d0fc9459f85e3df9d8b79b40c'
        self.link = 'https://st-api.test.yandex-team.ru'
        self.rep = registry.get_repository('startrek', 'issue_types',
                                           user_agent='ids-test',
                                           oauth2_access_token=self.token,
                                           server=self.link,
                                           fields_mapping={"new_name": "name"})
        self.startrek = self.rep.STARTREK_CONNECTOR_CLASS(self.rep.options)

    @pytest.mark.integration
    def test_getting_from_registry(self):
        self.assertEqual(self.rep.options['server'], self.link)
        self.assertEqual(self.rep.options['oauth2_access_token'], self.token)

        self.assertEqual(self.rep.get_user_session_id(), self.token)

        self.assertEqual(self.rep.SERVICE, 'startrek')
        self.assertEqual(self.rep.RESOURCES, 'issue_types')

        x = registry.get_repository('startrek', 'issue_types',
                                    user_agent='ids-test',
                                    oauth2_access_token=self.token,
                                    server=self.link)
        y = registry.get_repository('startrek', 'issue_types',
                                    user_agent='ids-test',
                                    oauth2_access_token=self.token,
                                    server=self.link)
        self.assertNotEqual(id(x), id(y))

    @pytest.mark.integration
    def test_user_fields_mapping(self):
        lookup = {"id": "4f6b271330049b12e72e3ef3"}
        obj = six.next(self.startrek.search_issue_types(lookup))
        res = self.rep._wrap_to_resource(obj)

        m = self.rep.options['fields_mapping']
        for field in m:
            self.assertEquals(res[field], res["__raw__"][m[field]])

    @pytest.mark.integration
    def test_get_with_lookup(self):
        ts = self.rep.get(lookup={'id': u'4f6b271330049b12e72e3ef3'})
        for t in ts:
            self.assertEqual(u'4f6b271330049b12e72e3ef3', t["id"])

    @pytest.mark.skip
    @pytest.mark.integration
    def test_update(self):
        t = self.rep.get_one(lookup={'id': '4f6b271330049b12e72e3ef3'})
        name = copy(t["new_name"])
        new_name = copy(name)
        new_name["ru"] = u"BUg"
        self.rep.update(t, fields={'new_name': new_name,
                                   'description': t['description']})
        self.assertEqual(t['new_name']['ru'], new_name["ru"])
        t = self.rep.get_one(lookup={'id': '4f6b271330049b12e72e3ef3'})
        self.assertEqual(t['new_name']['ru'], new_name["ru"])
        self.rep.update(t, fields={'new_name': name,
                                   'description': t['description']})
        self.assertEqual(t['new_name']['ru'], name["ru"])
        t = self.rep.get_one(lookup={'id': '4f6b271330049b12e72e3ef3'})
        self.assertEqual(t['new_name']['ru'], name["ru"])

        t = self.rep.get_one(lookup={'id': '4f6b271330049b12e72e3ef3'})
        tmp = t['description']
        self.rep.update(t, fields={'new_name': t['new_name'],
                                   'description': 'descr'})
        t = self.rep.get_one(lookup={'id': '4f6b271330049b12e72e3ef3'})
        self.assertEqual(t['description'], 'descr')
        self.rep.update(t, fields={'new_name': t['new_name'],
                                   'description': tmp})
        self.assertEqual(t['description'], tmp)
        t = self.rep.get_one(lookup={'id': '4f6b271330049b12e72e3ef3'})
        self.assertEqual(t['description'], tmp)

    @pytest.mark.skip
    @pytest.mark.integration
    def test_create(self):
        def get_and_check(fields):
            t = self.rep.create(fields=fields)
            p = self.rep.get_one(lookup={'id': t['id']})
            _assertEqualIssueTypes(t, p)
            return t

        name = {'en': 'test_issue_type', 'ru': 'test_issue_type_russian'}
        basic_fields = {
            'new_name': name,
            'description': 'New test-issue-type',
        }
        t = get_and_check(fields=basic_fields)
        self.assertEqual(t['new_name'], name)
        self.assertEqual(t['description'], basic_fields["description"])

    @pytest.mark.integration
    def test_delete(self):
        t = self.rep.get_one(lookup={})
        self.assertRaises(OperationNotPermittedError, self.rep.delete, t)
