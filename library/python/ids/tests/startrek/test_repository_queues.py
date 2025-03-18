# -*- coding: utf-8 -*-
import random
import string

import unittest
import pytest
from copy import copy

import six

from ids.exceptions import EmptyIteratorError, OperationNotPermittedError
from ids.registry import registry


def _assertEqualQueues(x, y):
    assert x['key'] == y['key']


def random_chars(n):
    return ''.join(random.choice(string.ascii_uppercase) for i in xrange(n))


class TestStartrekQueuesRepository(unittest.TestCase):
    def setUp(self):
        self.token = 'c24ce44d0fc9459f85e3df9d8b79b40c'
        self.link = 'https://st-api.test.yandex-team.ru'
        self.rep = registry.get_repository('startrek', 'queues',
                                           user_agent='ids-test',
                                           oauth2_access_token=self.token,
                                           server=self.link,
                                           fields_mapping={"new_name": "name"})
        self.startrek = self.rep.STARTREK_CONNECTOR_CLASS(self.rep.options)

    def _create_and_check(self, fields):
        queues = self.rep.get(lookup={})
        #find non-existing queue
        queue_keys = map(lambda queue: queue["key"], queues)
        i = 0
        while True:
            test_queue_key = "IDSTEST" + random_chars(4)
            if not test_queue_key in queue_keys:
                break
            i += 1
            if i > 20000:
                raise RuntimeError(
                    "Could not find an unused queue name, probably my "
                    "random name-generating code is rubbish")
        fields["key"] = test_queue_key
        t = self.rep.create(fields=fields)
        p = self.rep.get_one(lookup={'key': test_queue_key})
        _assertEqualQueues(t, p)
        return t, test_queue_key

    @pytest.mark.integration
    def test_getting_from_registry(self):
        self.assertEqual(self.rep.options['server'], self.link)
        self.assertEqual(self.rep.options['oauth2_access_token'], self.token)

        self.assertEqual(self.rep.get_user_session_id(), self.token)

        self.assertEqual(self.rep.SERVICE, 'startrek')
        self.assertEqual(self.rep.RESOURCES, 'queues')

        x = registry.get_repository('startrek', 'queues',
                                    user_agent='ids-test',
                                    server='http://localhost',
                                    oauth2_access_token="token")
        y = registry.get_repository('startrek', 'queues',
                                    user_agent='ids-test',
                                    server='http://localhost',
                                    oauth2_access_token="token")
        self.assertNotEqual(id(x), id(y))

    @pytest.mark.integration
    def test_user_fields_mapping(self):
        obj = six.next(self.startrek.search_queues({"key": "IDS"}))
        res = self.rep._wrap_to_resource(obj)

        m = self.rep.options['fields_mapping']
        for field in m:
            self.assertEquals(res[field], res["__raw__"][m[field]])

    @pytest.mark.integration
    def test_get_with_lookup(self):
        ts = self.rep.get(lookup={'key': u'IDS'})
        for t in ts:
            self.assertEqual(u'IDS', t["key"])

    @pytest.mark.integration
    def test_update(self):
        issue_type = six.next(self.startrek.search_issue_types({}))
        priority = six.next(self.startrek.search_priorities({}))
        name = 'IDS integration test QUEUE'
        basic_fields = {
            'new_name': name,
            'description': "ids.startrek.tests.TestStartrekQueuesRepository.tes"
                           "t_update\nhttps://github.yandex-team.ru/tools/ids/b"
                           "lob/master/packages/startrek/tests/repository_queue"
                           "s.py",
            'lead': {'login': 'zomb-prj-244'},
            'defaultType': {'id': issue_type["id"]},
            'defaultPriority': {'id': priority["id"]},
            'issueTypes': [{"id": issue_type["id"]}]
        }
        t, queue_key = self._create_and_check(fields=basic_fields)
        old_queue = copy(t)
        # TODO: enumerate queue fields
        del (old_queue["__repository__"])
        del (old_queue["__all__"])
        del (old_queue["__raw__"])
        new_queue = copy(old_queue)
        new_queue["new_name"] = "UPDATED IDS integration test QUEUE"

        self.rep.update(t, fields=new_queue)
        self.assertEqual(t['new_name'], new_queue["new_name"])
        t = self.rep.get_one(lookup={'key': queue_key})
        self.assertEqual(t['new_name'], new_queue["new_name"])
        self.rep.update(t, fields=old_queue)
        self.assertEqual(t['new_name'], old_queue["new_name"])
        t = self.rep.get_one(lookup={'key': queue_key})
        self.assertEqual(t['new_name'], old_queue["new_name"])

    @pytest.mark.integration
    def test_create(self):
        issue_type = six.next(self.startrek.search_issue_types({}))
        priority = six.next(self.startrek.search_priorities({}))
        name = 'IDS integration test QUEUE'
        basic_fields = {
            'new_name': name,
            'description': "ids.startrek.tests.TestStartrekQueuesRepository.tes"
                           "t_create\nhttps://github.yandex-team.ru/tools/ids/b"
                           "lob/master/packages/startrek/tests/repository_queue"
                           "s.py",
            'lead': {'login': 'zomb-prj-244'},
            'defaultType': {'id': issue_type["id"]},
            'defaultPriority': {'id': priority["id"]},
            'issueTypes': [{"id": issue_type["id"]}]
        }
        t, queue_key = self._create_and_check(fields=basic_fields)
        self.assertEqual(t['new_name'], name)
        self.assertEqual(t['description'], basic_fields["description"])

    @pytest.mark.integration
    def test_delete(self):
        t = self.rep.get_one(lookup={})
        self.assertRaises(OperationNotPermittedError, self.rep.delete, t)
