# -*- coding: utf-8 -*-

import unittest
import pytest
from copy import copy
from datetime import datetime

import six

from ids.exceptions import EmptyIteratorError, OperationNotPermittedError
from ids.registry import registry
from ids.repositories.bound_base import BoundRepositoryBase
from ids.resource import Resource
from ids.storages.null import NullStorage


def _assertEqualTickets(x, y):
    assert x['key'] == y['key'] and x['__raw__'].id == y['__raw__'].id


class TestJiraTicketsRepository(unittest.TestCase):
    def setUp(self):
        self.token = '5431b13a8cca4cc5b8b4cce46929d3aa'
        self.link = 'https://jira.test.tools.yandex-team.ru'
        self.rep = registry.get_repository('jira', 'tickets',
                            user_agent='ids-test',
                            oauth2_access_token=self.token,
                            server=self.link,
                            fields_mapping={
                                    'user': 'assignee',
                                    'planner_project_id': 'customfield_11830',
                                }
                        )
        self.jira = self.rep.JIRA_CONNECTOR_CLASS(self.link, self.token)
        self.jc = self.jira.connection

    @pytest.mark.integration
    def test_getting_from_registry(self):
        self.assertEqual(self.rep.options['server'], self.link)
        self.assertEqual(self.rep.options['oauth2_access_token'], self.token)

        self.assertEqual(self.rep.get_user_session_id(), self.token)

        self.assertEqual(self.rep.SERVICE, 'jira')
        self.assertEqual(self.rep.RESOURCES, 'tickets')

        x = registry.get_repository('jira', 'tickets', user_agent='ids-test')
        y = registry.get_repository('jira', 'tickets', user_agent='ids-test')
        self.assertNotEqual(id(x), id(y))

    @pytest.mark.integration
    def test_wrapping_to_resource(self):
        obj = self.jc.search_issues('key = PLAN-123')[0]
        res = self.rep._wrap_to_resource(obj)

        fields = [
            'assignee',
            'created',
            'updated',
            'description',
            'summary',
            'issuetype',
            'resolution',
            'resolutiondate',
            'project',
            'reporter',
            'status',
            'summary',
            'updated',

            'labels',
        ]
        for field in fields:
            self.assertEqual(res[field], obj.raw['fields'][field])

        fields = [
            #'rep_attachement',
            #'rep_components',
            #'rep_fix_versions',
            #'rep_labels',
            #'rep_subtasks',
            'rep_comments',
        ]
        for field in fields:
            self.assertTrue(field in res)
            self.assertTrue(isinstance(res[field], BoundRepositoryBase))

        self.assertEqual(res['key'], obj.raw['key'])
        self.assertEqual(res['id'], res['key'])
        self.assertEqual(res['__all__'], obj.raw['fields'])

    @pytest.mark.integration
    def test_user_fields_mapping(self):
        obj = self.jc.search_issues('key = PLAN-123')[0]
        res = self.rep._wrap_to_resource(obj)

        m = self.rep.options['fields_mapping']
        for field in m:
            self.assertTrue(field in res)
            self.assertTrue(m[field] in res)
            self.assertEqual(res[field], res[m[field]])

    @pytest.mark.integration
    def test_get_with_different_queries(self):
        lookup = {'key': 'PLAN-1'}

        t = self.rep.get(lookup)[0]
        self.assertTrue(t is not None)
        self.assertEqual(t['key'], lookup['key'])
        p = self.rep.get(lookup)[0]
        self.assertEqual(p['key'], lookup['key'])
        _assertEqualTickets(t, p)
        self.assertTrue(isinstance(t, Resource))
        self.assertTrue(isinstance(p, Resource))

        t = self.rep.get_one(lookup)
        self.assertEqual(t['key'], lookup['key'])
        _assertEqualTickets(t, p)
        p = self.rep.get_one(lookup)
        self.assertEqual(p['key'], lookup['key'])
        _assertEqualTickets(t, p)
        self.assertTrue(isinstance(t, Resource))
        self.assertTrue(isinstance(p, Resource))

        it = self.rep.getiter(lookup)
        self.assertFalse(isinstance(it, Resource))
        t = six.next(it)
        self.assertEqual(t['key'], lookup['key'])
        _assertEqualTickets(t, p)
        it = self.rep.getiter(lookup)
        self.assertFalse(isinstance(it, Resource))
        p = six.next(it)
        self.assertEqual(p['key'], lookup['key'])
        _assertEqualTickets(t, p)
        self.assertTrue(isinstance(t, Resource))
        self.assertTrue(isinstance(p, Resource))

    @pytest.mark.integration
    def test_get_with_different_options(self):
        lookup = {'key': 'PLAN-1'}

        self.assertRaises(EmptyIteratorError, self.rep.get_one, lookup,
                                                use_service=False,
                                                use_storage=False,
                            )

        t = self.rep.get_one(lookup,
                                use_service=True,
                                use_storage=False,
                                update_storage=False,
                            )
        self.assertTrue(t is not None)
        self.assertEqual(t['key'], lookup['key'])

        self.assertRaises(EmptyIteratorError, self.rep.get_one, lookup,
                                                use_service=False,
                                                use_storage=False,
                            )

        p = self.rep.get_one(lookup,
                                use_service=True,
                                use_storage=False,
                                update_storage=True,
                            )
        self.assertTrue(p is not None)
        self.assertEqual(p['key'], lookup['key'])
        _assertEqualTickets(t, p)

        # если используется хранилище, то протестируем и его
        if not isinstance(self.rep.storage, NullStorage):
            t = self.rep.get_one(lookup,
                                    use_service=False,
                                    use_storage=True,
                                )
            self.assertTrue(t is not None)
            self.assertEqual(t['key'], lookup['key'])
            _assertEqualTickets(t, p)

            key = self.rep._make_storage_key({'key': 'PLAN-2'})
            self.assertTrue(self.rep.storage.get(key) is None)
            p = self.rep.get_one(lookup,
                                    use_service=True,
                                    use_storage=True,
                                    update_storage=True,
                                )
            self.assertTrue(p is not None)
            self.assertEqual(p['key'], lookup['key'])
            self.assertTrue(self.rep.storage.get(key) is not None)

    @pytest.mark.integration
    def test_get_with_different_lookup(self):
        t = self.rep.create(fields={
                                    'project': {'key': 'PLAN'},
                                    'summary': 'New test-issue',
                                })
        self.rep.update(t, fields={'planner_project_id': 666})
        t = self.rep.get_one(lookup={'planner_project_id': 666})
        self.assertEqual(int(t['planner_project_id']), 666)
        self.assertEqual(int(t[self.rep.options['fields_mapping']['planner_project_id']]), 666)

        ts = self.rep.get(lookup={'status': 'reopened'})
        for t in ts:
            self.assertEqual(t['status']['id'], '4')

        ts = self.rep.getiter(lookup={'key__in': ['PLAN-666', 'PLAN-123']})
        for t in ts:
            self.assertTrue(t['key'] in ['PLAN-666', 'PLAN-123'])

    @pytest.mark.integration
    def test_get_with_different_fields(self):
        # 2*work_days*work_hours*secs_in_hour
        work_seconds_two_week = 2 * 5 * 8 * 60 * 60

        t = self.rep.get_one(lookup={'affectedVersion': '2007Q1'})
        self.assertEqual(t['__all__']['versions'][0]['name'], '2007Q1')

        t = self.rep.get_one(lookup={'assignee': 'mixael'})
        self.assertEqual(t['assignee']['name'], 'mixael')

        ts = self.rep.get_one(lookup={'comment__contains': 'comment'})
        self.assertTrue(len(ts) > 0)

        t = self.rep.get_one(lookup={'component': 'Web Interface'})
        self.assertEqual(t['__all__']['components'][0]['name'], 'Web Interface')

        t = self.rep.get_one(lookup={'created__gt': '2012-10-10'})
        p = self.rep.get_one(lookup={'createdDate__gt': '2012-10-10'})
        self.assertTrue(datetime.strptime(t['created'][:10], '%Y-%m-%d') >=
                        datetime.strptime('2012-10-10', '%Y-%m-%d'))
        _assertEqualTickets(t, p)

        t = self.rep.get_one(lookup={'description__contains': u'сниппет'})
        self.assertTrue(u'сниппет' in t['description'])

        t = self.rep.get_one(lookup={'environment__contains': 'Firefox 3'})
        self.assertTrue('Firefox 3' in t['environment'])

        t = self.rep.get_one(lookup={'issue': 'PLAN-123'})
        p = self.rep.get_one(lookup={'issueKey': 'PLAN-123'})
        k = self.rep.get_one(lookup={'key': 'PLAN-123'})
        w = self.rep.get_one(lookup={'id': 'PLAN-123'})
        self.assertEqual(t['key'], 'PLAN-123')
        self.assertEqual(p['id'], 'PLAN-123')
        _assertEqualTickets(t, p)
        _assertEqualTickets(p, k)
        _assertEqualTickets(k, w)

        t = self.rep.get_one(lookup={'labels': 'test'})
        self.assertTrue('test' in t['labels'])

        t = self.rep.get_one(lookup={'originalEstimate': '2w'})
        self.assertEqual(t['__all__']['timeoriginalestimate'], work_seconds_two_week)

        t = self.rep.get_one(lookup={'originalEstimate': '2w'})
        p = self.rep.get_one(lookup={'timeOriginalEstimate': '2w'})
        self.assertEqual(t['__all__']['timeoriginalestimate'], work_seconds_two_week)
        _assertEqualTickets(t, p)

        t = self.rep.get_one(lookup={'key': 'PLAN-127'})
        self.assertEqual(t['id'], 'PLAN-127')
        self.assertEqual(t['parent']['key'], 'PLAN-123')

        t = self.rep.get_one(lookup={'priority': 'Trivial'})
        self.assertEqual(t['priority']['id'], '5')

        t = self.rep.get_one(lookup={'project': 'PLAN'})
        self.assertEqual(t['project']['key'], 'PLAN')

        t = self.rep.get_one(lookup={'remainingEstimate': '2w'})
        p = self.rep.get_one(lookup={'timeEstimate': '2w'})
        self.assertEqual(t['__all__']['timeestimate'], work_seconds_two_week)
        _assertEqualTickets(t, p)

        t = self.rep.get_one(lookup={'reporter': 'mixael'})
        self.assertEqual(t['reporter']['name'], 'mixael')

        t = self.rep.get_one(lookup={'resolution': 'Will Not Fix'})
        self.assertEqual(t['resolution']['id'], '2')

        t = self.rep.get_one(lookup={'resolved__gt': '2012-10-10'})
        p = self.rep.get_one(lookup={'resolutionDate__gt': '2012-10-10'})
        self.assertTrue(datetime.strptime(t['resolutiondate'][:10], '%Y-%m-%d') >=
                        datetime.strptime('2012-10-10', '%Y-%m-%d'))
        _assertEqualTickets(t, p)

        t = self.rep.get_one(lookup={'status': 'In Progress'})
        self.assertEqual(t['status']['id'], '3')

        t = self.rep.get_one(lookup={'summary__contains': u'список'})
        self.assertTrue(u'список' in t['summary'])

        t = self.rep.get_one(lookup={'type': 'New Feature'})
        p = self.rep.get_one(lookup={'issueType': 'New Feature'})
        self.assertEqual(t['issuetype']['id'], '2')
        _assertEqualTickets(t, p)

        t = self.rep.get_one(lookup={'timeSpent': '2w'})
        self.assertEqual(t['__all__']['timespent'], work_seconds_two_week)

        t = self.rep.get_one(lookup={'updated__gt': '2011-10-10'})
        p = self.rep.get_one(lookup={'updatedDate__gt': '2011-10-10'})
        self.assertTrue(datetime.strptime(t['updated'][:10], '%Y-%m-%d') >=
                        datetime.strptime('2011-10-10', '%Y-%m-%d'))
        _assertEqualTickets(t, p)

        t = self.rep.get_one(lookup={'workRatio': 1})
        self.assertEqual(t['__all__']['workratio'], 1)

    @pytest.mark.integration
    def test_update(self):
        first = self.rep.get_one(lookup={})
        self.rep.update(first, fields={'planner_project_id': 123})

        t = self.rep.get_one(lookup={'planner_project_id': 123})
        self.assertEqual(int(t['planner_project_id']), 123)
        _assertEqualTickets(t, first)

        self.rep.update(t, fields={'planner_project_id': 666})
        self.assertEqual(int(t['planner_project_id']), 666)

        t = self.rep.get_one(lookup={'key': 'PLAN-123'})

        tmp = t['assignee']
        self.rep.update(t, fields={'assignee': {'name': 'mixael'}})
        self.assertEqual(t['assignee']['name'], 'mixael')
        self.rep.update(t, fields={'assignee': {'name': tmp['name']}})
        self.assertEqual(t['assignee'], tmp)

        tmp = t['description']
        self.rep.update(t, fields={'description': 'descr'})
        self.assertEqual(t['description'], 'descr')
        self.rep.update(t, fields={'description': tmp})
        self.assertEqual(t['description'], tmp)

        tmp = t['environment']
        self.rep.update(t, fields={'environment': 'env'})
        self.assertEqual(t['environment'], 'env')
        self.rep.update(t, fields={'environment': tmp})
        self.assertEqual(t['environment'], tmp)

        tmp = t['issuetype']
        self.rep.update(t, fields={'issuetype': {'name': 'New Feature'}})
        self.assertEqual(t['issuetype']['name'], 'New Feature')
        self.rep.update(t, fields={'issuetype': {'id': tmp['id']}})
        self.assertEqual(t['issuetype'], tmp)

        tmp = t['labels']
        self.rep.update(t, fields={'labels': ['label', 'test']})
        self.assertEqual(t['labels'], ['label', 'test'])
        self.rep.update(t, fields={'labels': tmp})
        self.assertEqual(t['labels'], tmp)

        tmp = t['priority']
        self.rep.update(t, fields={'priority': {'name': 'Minor'}})
        self.assertEqual(t['priority']['name'], 'Minor')
        self.rep.update(t, fields={'priority': {'id': tmp['id']}})
        self.assertEqual(t['priority'], tmp)

        tmp = t['reporter']
        self.rep.update(t, fields={'reporter': {'name': 'dword'}})
        self.assertEqual(t['reporter']['name'], 'dword')
        self.rep.update(t, fields={'reporter': {'name': tmp['name']}})
        self.assertEqual(t['reporter'], tmp)

        tmp = t['summary']
        self.rep.update(t, fields={'summary': 'new summary'})
        self.assertEqual(t['summary'], 'new summary')
        self.rep.update(t, fields={'summary': tmp})
        self.assertEqual(t['summary'], tmp)

    @pytest.mark.integration
    def test_create(self):
        def get_and_check(fields):
            t = self.rep.create(fields=fields)
            p = self.rep.get_one(lookup={'key': t['key']})
            _assertEqualTickets(t, p)
            return t

        basic_fields = {
            'project': {'key': 'PLAN'},
            'summary': 'New test-issue',
        }

        t = get_and_check(fields=basic_fields)
        self.assertEqual(t['project']['key'], 'PLAN')
        self.assertEqual(t['summary'], 'New test-issue')

        basic_fields.update({'issuetype': {'name': 'Bug'}})
        t = get_and_check(fields=basic_fields)
        self.assertEqual(t['issuetype']['id'], '1')

        fields = copy(basic_fields)
        fields.update({'assignee': {'name': 'dword'}})
        t = get_and_check(fields=fields)
        self.assertEqual(t['assignee']['name'], 'dword')
        self.assertEqual(t['assignee']['emailAddress'], 'dword@yandex-team.ru')

        fields = copy(basic_fields)
        fields.update({'description': 'some test description'})
        t = get_and_check(fields=fields)
        self.assertEqual(t['description'], 'some test description')

        fields = copy(basic_fields)
        fields.update({'environment': 'some test environment'})
        t = get_and_check(fields=fields)
        self.assertEqual(t['environment'], 'some test environment')

        fields = copy(basic_fields)
        fields.update({'reporter': {'name': 'dword'}})
        t = get_and_check(fields=fields)
        self.assertEqual(t['reporter']['name'], 'dword')
        self.assertEqual(t['reporter']['emailAddress'], 'dword@yandex-team.ru')

        fields = copy(basic_fields)
        fields.update({'priority': {'name': 'Critical'}})
        t = get_and_check(fields=fields)
        self.assertEqual(t['priority']['name'], 'Critical')
        self.assertEqual(t['priority']['id'], '2')

        fields = copy(basic_fields)
        fields.update({'labels': ['label', 'test']})
        t = get_and_check(fields=fields)
        self.assertEqual(t['labels'], ['label', 'test'])

        fields = copy(basic_fields)
        fields.update({'issuetype': {'name': 'Sub-task'}})
        fields.update({'parent': {'key': 'PLAN-123'}})
        t = get_and_check(fields=fields)
        self.assertEqual(t['issuetype']['id'], '5')
        self.assertEqual(t['parent']['key'], 'PLAN-123')

    @pytest.mark.integration
    def test_delete(self):
        t = self.rep.get_one(lookup={})
        # тикеты не удаляются из джиры
        self.assertRaises(OperationNotPermittedError, self.rep.delete, t)
