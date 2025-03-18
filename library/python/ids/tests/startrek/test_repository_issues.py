# -*- coding: utf-8 -*-

import unittest
import pytest
from copy import copy

import six

from ids.exceptions import EmptyIteratorError, OperationNotPermittedError
from ids.registry import registry
from ids.repositories.bound_base import BoundRepositoryBase
from ids.resource import Resource
from ids.storages.null import NullStorage


def _assertEqualIssues(x, y):
    assert x['key'] == y['key'] and x['id'] == y['id']



class TestStartrekIssuesRepository(unittest.TestCase):
    def setUp(self):
        self.token = 'c24ce44d0fc9459f85e3df9d8b79b40c'
        self.link = 'https://st-api.test.yandex-team.ru'
        self.rep = registry.get_repository('startrek', 'issues',
                                           user_agent='ids-test',
                                           oauth2_access_token=self.token,
                                           server=self.link,
                                           fields_mapping={
                                               'user': 'assignee',
                                               'reporter': 'author',
                                               'issueType': 'type',
                                               'resolutiondate': 'resolved',
                                               'labels': 'tags'
                                           })
        self.startrek = self.rep.STARTREK_CONNECTOR_CLASS(self.rep.options)

    @pytest.mark.integration
    def test_getting_from_registry(self):
        self.assertEqual(self.rep.options['server'], self.link)
        self.assertEqual(self.rep.options['oauth2_access_token'], self.token)

        self.assertEqual(self.rep.get_user_session_id(), self.token)

        self.assertEqual(self.rep.SERVICE, 'startrek')
        self.assertEqual(self.rep.RESOURCES, 'issues')

        x = registry.get_repository('startrek', 'issues',
                                    user_agent='ids-test',
                                    oauth2_access_token=self.token,
                                    server=self.link)
        y = registry.get_repository('startrek', 'issues',
                                    user_agent='ids-test',
                                    oauth2_access_token=self.token,
                                    server=self.link)
        self.assertNotEqual(id(x), id(y))

    @pytest.mark.integration
    def test_wrapping_to_resource(self):
        obj = six.next(self.startrek.search_issues({'key': 'IDS-1'}))
        res = self.rep._wrap_to_resource(obj)
        fields = [
            'user',
            'created',
            'updated',
            'description',
            'summary',
            'issueType',
            'resolution',
            'resolutiondate',
            'project',
            'reporter',
            'status',
            'summary',
            'updated',

            'labels',
        ]
        fields_mapping = self.rep.options.get("fields_mapping", {})
        for field in fields:
            startrek_field_name = field
            if field in fields_mapping:
                startrek_field_name = fields_mapping[field]
            self.assertEqual(res[field], obj[startrek_field_name])

        fields = [
            #'rep_attachement',
            #'rep_components',
            #'rep_fix_versions',
            #'rep_labels',
            #'rep_subtasks',
            #'rep_comments',
        ]
        for field in fields:
            self.assertTrue(field in res)
            self.assertTrue(isinstance(res[field], BoundRepositoryBase))

        self.assertEqual(res['key'], obj['key'])
        self.assertEqual(res['id'], obj['id'])

    @pytest.mark.integration
    def test_user_fields_mapping(self):
        obj = six.next(self.startrek.search_issues({"key": "IDS-1"}))
        res = self.rep._wrap_to_resource(obj)

        m = self.rep.options['fields_mapping']
        for field in m:
            self.assertEquals(res[field], res["__raw__"][m[field]])

    @pytest.mark.integration
    def test_get_with_different_queries(self):
        lookup = {'key': 'IDS-1'}

        t = self.rep.get(lookup)[0]
        self.assertTrue(t is not None)
        self.assertEqual(t['key'], lookup['key'])

        p = self.rep.get(lookup)[0]
        self.assertEqual(p['key'], lookup['key'])
        _assertEqualIssues(t, p)
        self.assertTrue(isinstance(t, Resource))
        self.assertTrue(isinstance(p, Resource))

        t = self.rep.get_one(lookup)
        self.assertEqual(t['key'], lookup['key'])
        _assertEqualIssues(t, p)
        p = self.rep.get_one(lookup)
        self.assertEqual(p['key'], lookup['key'])
        _assertEqualIssues(t, p)
        self.assertTrue(isinstance(t, Resource))
        self.assertTrue(isinstance(p, Resource))

        it = self.rep.getiter(lookup)
        self.assertFalse(isinstance(it, Resource))
        t = six.next(it)
        self.assertEqual(t['key'], lookup['key'])
        _assertEqualIssues(t, p)
        it = self.rep.getiter(lookup)
        self.assertFalse(isinstance(it, Resource))
        p = six.next(it)
        self.assertEqual(p['key'], lookup['key'])
        _assertEqualIssues(t, p)
        self.assertTrue(isinstance(t, Resource))
        self.assertTrue(isinstance(p, Resource))

    @pytest.mark.integration
    def test_get_with_different_options(self):
        lookup = {'key': 'IDS-1'}

        self.assertRaises(EmptyIteratorError, self.rep.get_one, lookup,
                          use_service=False,
                          use_storage=False, )

        t = self.rep.get_one(lookup,
                             use_service=True,
                             use_storage=False,
                             update_storage=False, )
        self.assertTrue(t is not None)
        self.assertEqual(t['key'], lookup['key'])

        self.assertRaises(EmptyIteratorError, self.rep.get_one, lookup,
                          use_service=False,
                          use_storage=False, )

        p = self.rep.get_one(lookup,
                             use_service=True,
                             use_storage=False,
                             update_storage=True, )
        self.assertTrue(p is not None)
        self.assertEqual(p['key'], lookup['key'])
        _assertEqualIssues(t, p)

        # если используется хранилище, то протестируем и его
        if not isinstance(self.rep.storage, NullStorage):
            t = self.rep.get_one(lookup,
                                 use_service=False,
                                 use_storage=True, )
            self.assertTrue(t is not None)
            self.assertEqual(t['key'], lookup['key'])
            _assertEqualIssues(t, p)

            key = self.rep._make_storage_key({'key': 'IDS-2'})
            self.assertTrue(self.rep.storage.get(key) is None)
            p = self.rep.get_one(lookup,
                                 use_service=True,
                                 use_storage=True,
                                 update_storage=True, )
            self.assertTrue(p is not None)
            self.assertEqual(p['key'], lookup['key'])
            self.assertTrue(self.rep.storage.get(key) is not None)

    @pytest.mark.integration
    @pytest.mark.skip(reason="This test takes forever to complete - it "
                             "fetches too many tickets. Probably not a good "
                             "idea.")
    def test_get_with_different_lookup(self):
        t = self.rep.create(fields={
            'queue': {'id': '50ddeae0e4b0c2036ae8c24a'},
            'summary': 'IDS integration tests ISSUE',
            'description': 'ids.startrek.tests.TestStartrekIssuesRepository.tes'
                           't_get_with_different_lookup\nhttps://github.yandex-'
                           'team.ru/tools/ids/blob/master/packages/startrek/tes'
                           'ts/startrek_connector.py'
        })

        ts = self.rep.get(lookup={'status': '4f6b29673004ccc027df1e1c',
                                  'key': 'TEST-1'})
        for t in ts:
            self.assertEqual(t['status']['key'], 'open')
            break  # yes, break

    @pytest.mark.integration
    def test_get_with_different_fields(self):

        t = self.rep.get_one(lookup={'fixVersions': '502270d2e4b0537d39d32650'})
        self.assertEqual(t['__all__']['fixVersions'][0]['name'], '1.0')

        t = self.rep.get_one(lookup={'user': 'mixael'})
        self.assertEqual(t['user']['login'], 'mixael')

        t = self.rep.get_one(lookup={'components': '50768da4e4b0f6cf4f59efd3'})
        self.assertEqual(t['__all__']['components'][0]['id'],
                         '50768da4e4b0f6cf4f59efd3')

        t = self.rep.get_one(lookup={'description': u'сниппет'})
        self.assertTrue(u'сниппет' in t['description'])

        t = self.rep.get_one(lookup={'labels': u'аватар'})
        self.assertTrue(u'аватар' in t['labels'])

        t = self.rep.get_one(lookup={'priority': '4f6b27fd30047cf5e726d108'})
        self.assertEqual(t['priority']['key'], 'normal')

        t = self.rep.get_one(lookup={'reporter': 'mixael'})
        self.assertEqual(t['reporter']['login'], 'mixael')

        t = self.rep.get_one(lookup={'resolution': '4f6b29cc3004e9443e453da7'})
        self.assertEqual(t['resolution']['key'], 'fixed')

        t = self.rep.get_one(lookup={'status': '4f6b29673004ccc027df1e53'})
        self.assertEqual(t['status']['key'], 'closed')

        t = self.rep.get_one(lookup={'type': '4f6b271330049b12e72e3f07'})
        p = self.rep.get_one(lookup={'issueType': '4f6b271330049b12e72e3f07'})
        self.assertEqual(t['issueType']['key'], 'task')
        _assertEqualIssues(t, p)

    @pytest.mark.integration
    def test_update(self):
        t = self.rep.get_one(lookup={'key': 'IDS-1'})

        tmp = t['user']
        self.rep.update(t, fields={'user': {'login': 'mixael'},
                                   'version': t['version']})
        self.assertEqual(t['user']['login'], 'mixael')
        self.rep.update(t, fields={'user': {'login': tmp['login']},
                                   'version': t['version']})
        self.assertEqual(t['user'], tmp)

        tmp = t['description']
        self.rep.update(t, fields={'description': 'descr',
                                   'version': t['version']})
        self.assertEqual(t['description'], 'descr')
        self.rep.update(t, fields={'description': tmp, 'version': t['version']})
        self.assertEqual(t['description'], tmp)

        tmp = t['issueType']
        self.rep.update(t, fields={'issueType': {
            'id': '4f6b271330049b12e72e3ef3'
        }, 'version': t['version']})
        self.assertEqual(t['issueType']['key'], 'bug')
        self.rep.update(t, fields={'issueType': {'id': tmp['id']},
                                   'version': t['version']})
        self.assertEqual(t['issueType'], tmp)

        tmp = t['labels']
        self.rep.update(t, fields={'labels': ['label', 'test'],
                                   'version': t['version']})
        self.assertEqual(t['labels'], ['label', 'test'])
        self.rep.update(t, fields={'labels': tmp, 'version': t['version']})
        self.assertEqual(t['labels'], tmp)

        tmp = t['priority']
        self.rep.update(t,
                        fields={'priority': {'id': '4f6b27fd30047cf5e726d105'},
                                'version': t['version']})
        self.assertEqual(t['priority']['key'], 'blocker')
        self.rep.update(t, fields={'priority': {'id': tmp['id']},
                                   'version': t['version']})
        self.assertEqual(t['priority'], tmp)

        tmp = t['reporter']
        self.rep.update(t, fields={'reporter': {'login': 'dword'},
                                   'version': t['version']})
        self.assertEqual(t['reporter']['login'], 'dword')
        self.rep.update(t, fields={'reporter': {'login': tmp['login']},
                                   'version': t['version']})
        self.assertEqual(t['reporter'], tmp)

        tmp = t['summary']
        self.rep.update(t, fields={'summary': 'new summary',
                                   'version': t['version']})
        self.assertEqual(t['summary'], 'new summary')
        self.rep.update(t, fields={'summary': tmp, 'version': t['version']})
        self.assertEqual(t['summary'], tmp)

    @pytest.mark.integration
    def test_create(self):
        def get_and_check(fields):
            t = self.rep.create(fields=fields)
            p = self.rep.get_one(lookup={'key': t['key']})
            _assertEqualIssues(t, p)
            return t

        queue_key = "IDS"
        queue = six.next(self.startrek.search_queues({"key": queue_key}))

        summary = 'IDS integration tests ISSUE'
        description = ('ids.startrek.tests.TestStartrekIssuesRepository.test_cr'
                       'eate\nhttps://github.yandex-team.ru/tools/ids/blob/mast'
                       'er/packages/startrek/tests/startrek_connector.py')
        basic_fields = {
            'queue': {'id': queue["id"]},
            'summary': summary,
            'description': description
        }
        t = get_and_check(fields=basic_fields)
        self.assertEqual(t['queue']['key'], queue_key)
        self.assertEqual(t['summary'], summary)

        lookup = {'id': '4f6b271330049b12e72e3ef3'}
        issue_type = six.next(self.startrek.search_issue_types(lookup))
        basic_fields.update({'issueType': {"id": issue_type["id"]}})
        t = get_and_check(fields=basic_fields)
        self.assertEqual(t['issueType']['key'], issue_type["key"])

        fields = copy(basic_fields)
        fields.update({'user': {'login': 'dword'}})
        t = get_and_check(fields=fields)
        self.assertEqual(t['user']['login'], 'dword')
        self.assertEqual(t['user']['email'], 'dword@yandex-team.ru')

        fields = copy(basic_fields)
        fields.update({'description': 'some test description'})
        t = get_and_check(fields=fields)
        self.assertEqual(t['description'], 'some test description')

        fields = copy(basic_fields)
        t = get_and_check(fields=fields)
        self.assertEqual(t['reporter']['login'], 'zomb-prj-244')
        assert t['reporter']['email'] is None

        fields = copy(basic_fields)
        fields.update({'priority': {'id': '4f6b27fd30047cf5e726d106'}})
        t = get_and_check(fields=fields)
        self.assertEqual(t['priority']['key'], 'critical')

        fields = copy(basic_fields)
        fields.update({'labels': ['label', 'test']})
        t = get_and_check(fields=fields)
        self.assertEqual(t['labels'], ['label', 'test'])

    @pytest.mark.integration
    def test_delete(self):
        t = self.rep.get_one(lookup={})
        self.assertRaises(OperationNotPermittedError, self.rep.delete, t)

    @pytest.mark.integration
    def test_get_by_key_list(self):
        '''Проверить балк-версию ручки v1/issues'''
        lookup = {'key': ['IDS-1', 'IDS-2', 'IDS-3']}
        issues = self.rep.get(lookup)
        self.assertTrue(len(issues), len(lookup['key']))

    @pytest.mark.integration
    def test_get_by_key_list2(self):
        '''Проверить балк-версию ручки v1/issues - 2'''
        lookup = {'key': ['IDS-1']}
        issues = self.rep.get(lookup)
        self.assertTrue(len(issues), len(lookup['key']))
