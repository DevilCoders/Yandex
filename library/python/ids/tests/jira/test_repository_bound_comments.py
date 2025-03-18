# -*- coding: utf-8 -*-

import unittest
import pytest
import six

from ids.exceptions import EmptyIteratorError, KeyIsAbsentError
from ids.registry import registry
from ids.repositories.bound_base import BoundRepositoryBase
from ids.resource import Resource
from ids.storages.null import NullStorage


def _assertEqualComments(x, y):
    assert (set(x.keys()) == set(y.keys()) and
            x['__raw__'].raw['id'] == y['__raw__'].raw['id'])


class TestJiraCommentsBoundRepository(unittest.TestCase):
    def setUp(self):
        self.token = '5431b13a8cca4cc5b8b4cce46929d3aa'
        self.link = 'https://jira.test.tools.yandex-team.ru'
        self.rep = registry.get_repository('jira', 'tickets',
                            user_agent='ids-test',
                            oauth2_access_token=self.token,
                            server=self.link,
                        )

        self.parent = self.rep.get_one(lookup={'key': 'PLAN-123'})
        self.comments = self.parent['rep_comments']
        self.comments.options['fields_mapping'] = {'text': 'body'}

        self.jira = self.rep.JIRA_CONNECTOR_CLASS(self.link, self.token)
        self.jc = self.jira.connection

    @pytest.mark.integration
    def test_getting_from_parent_resource(self):
        self.assertRaises(KeyIsAbsentError, registry.get_repository,
                          'jira', 'comments', user_agent='ids-test')

        self.assertTrue(self.comments is not None)
        self.assertTrue(isinstance(self.comments, BoundRepositoryBase))

        self.assertEqual(self.comments.SERVICE, 'jira')
        self.assertEqual(self.comments.RESOURCES, 'comments')

    @pytest.mark.integration
    def test_wrapping_to_resource(self):
        obj = self.jc.comments('PLAN-123')[0]
        res = self.comments._wrap_to_resource(obj)
        self.assertEqual(res['__parent__'], self.parent)

        fields = [
            'author',
            'body',
            'created',
            'updateAuthor',
            'updated',
        ]
        for field in fields:
            self.assertEqual(res[field], obj.raw[field])

        self.assertEqual(res['__all__'], obj.raw)

    @pytest.mark.integration
    def test_user_fields_mapping(self):
        obj = self.jc.comments('PLAN-123')[0]
        res = self.comments._wrap_to_resource(obj)

        m = self.rep.options['fields_mapping']
        for field in m:
            self.assertTrue(field in res)
            self.assertTrue(m[field] in res)
            self.assertEqual(res[field], res[m[field]])

    @pytest.mark.integration
    def test_get_with_different_queries(self):
        lookup = {'author__name': 'mixael'}

        t = self.comments.get(lookup)[0]
        self.assertTrue(t is not None)
        self.assertEqual(t['author']['name'], lookup['author__name'])
        p = self.comments.get(lookup)[0]
        self.assertEqual(p['author']['name'], lookup['author__name'])
        _assertEqualComments(t, p)
        self.assertTrue(isinstance(t, Resource))
        self.assertTrue(isinstance(p, Resource))

        t = self.comments.get_one(lookup)
        self.assertEqual(t['author']['name'], lookup['author__name'])
        _assertEqualComments(t, p)
        p = self.comments.get_one(lookup)
        self.assertEqual(p['author']['name'], lookup['author__name'])
        _assertEqualComments(t, p)
        self.assertTrue(isinstance(t, Resource))
        self.assertTrue(isinstance(p, Resource))

        it = self.comments.getiter(lookup)
        self.assertFalse(isinstance(it, Resource))
        t = six.next(it)
        self.assertEqual(t['author']['name'], lookup['author__name'])
        _assertEqualComments(t, p)
        it = self.comments.getiter(lookup)
        self.assertFalse(isinstance(it, Resource))
        p = six.next(it)
        self.assertEqual(p['author']['name'], lookup['author__name'])
        _assertEqualComments(t, p)
        self.assertTrue(isinstance(t, Resource))
        self.assertTrue(isinstance(p, Resource))

    @pytest.mark.integration
    def test_get_with_different_options(self):
        lookup = {'author__name': 'mixael'}

        self.assertRaises(EmptyIteratorError, self.comments.get_one, lookup,
                                                use_service=False,
                                                use_storage=False,
                            )

        t = self.comments.get_one(lookup,
                                use_service=True,
                                use_storage=False,
                                update_storage=False,
                            )
        self.assertTrue(t is not None)
        self.assertEqual(t['author']['name'], lookup['author__name'])

        self.assertRaises(EmptyIteratorError, self.comments.get_one, lookup,
                                                use_service=False,
                                                use_storage=False,
                            )

        p = self.comments.get_one(lookup,
                                use_service=True,
                                use_storage=False,
                                update_storage=True,
                            )
        self.assertTrue(p is not None)
        self.assertEqual(p['author']['name'], lookup['author__name'])
        _assertEqualComments(t, p)

        # если используется хранилище, то протестируем и его
        if not isinstance(self.comments.storage, NullStorage):
            t = self.comments.get_one(lookup,
                                    use_service=False,
                                    use_storage=True,
                                )
            self.assertTrue(t is not None)
            self.assertEqual(t['author']['name'], lookup['author__name'])
            _assertEqualComments(t, p)

            key = self.comments._make_storage_key({'text__contains': 'qw'})
            self.assertTrue(self.comments.storage.get(key) is None)
            p = self.comments.get_one(lookup,
                                    use_service=True,
                                    use_storage=True,
                                    update_storage=True,
                                )
            self.assertTrue(p is not None)
            self.assertTrue(lookup['text__contains'] in p['text'])
            self.assertTrue(self.comments.storage.get(key) is not None)

    @pytest.mark.integration
    def test_get_with_different_lookup(self):
        ts = self.comments.getiter(lookup={'author__name': 'mixael'})
        for t in ts:
            self.assertEqual(t['author']['name'], 'mixael')

        t = self.comments.get_one(lookup={'author__emailAddress': 'mixael@yandex-team.ru'})
        self.assertEqual(t['author']['emailAddress'], 'mixael@yandex-team.ru')

        ts = self.comments.get(lookup={'author__displayName': u'Петров Михаил'})
        for t in ts:
            self.assertEqual(t['author']['displayName'], u'Петров Михаил')

        t = self.comments.get_one(lookup={'author__active': True})
        self.assertEqual(t['author']['active'], True)

        t = self.comments.get_one(lookup={'text': 'qwe'})
        self.assertEqual(t['text'], 'qwe')
        self.assertEqual(t[self.comments.options['fields_mapping']['text']], 'qwe')

        # TODO: парсинг таймзоны (через dateutil.parser.parse)
        t = self.comments.get_one(lookup={'created': '2012-10-25T20:00:54.000+0400'})
        self.assertEqual(t['created'], '2012-10-25T20:00:54.000+0400')

        t = self.comments.get_one(lookup={'updated': '2012-10-25T20:00:54.000+0400'})
        self.assertEqual(t['updated'], '2012-10-25T20:00:54.000+0400')

        ts = self.comments.getiter(lookup={'updateAuthor__name': 'mixael'})
        for t in ts:
            self.assertEqual(t['updateAuthor']['name'], 'mixael')

        t = self.comments.get_one(lookup={'updateAuthor__emailAddress': 'mixael@yandex-team.ru'})
        self.assertEqual(t['updateAuthor']['emailAddress'], 'mixael@yandex-team.ru')

        ts = self.comments.get(lookup={'updateAuthor__displayName': u'Петров Михаил'})
        for t in ts:
            self.assertEqual(t['updateAuthor']['displayName'], u'Петров Михаил')

        t = self.comments.get_one(lookup={'updateAuthor__active': True})
        self.assertEqual(t['updateAuthor']['active'], True)

    @pytest.mark.integration
    def test_update(self):
        first = self.comments.get_one(lookup={})
        txt = first['text']
        self.comments.update(first, fields={'text': 'suffix'})

        t = self.comments.get_one(lookup={'text': 'suffix'})
        self.assertEqual(t['text'], 'suffix')
        _assertEqualComments(t, first)

        self.comments.update(t, fields={'text': txt})
        self.assertEqual(t['text'], txt)

    @pytest.mark.integration
    def test_create(self):
        L = len(self.comments.get(lookup={}))
        txt = 'comment{0}'.format(L)
        t = self.comments.create(fields={'text': txt})
        self.assertEqual(t['body'], txt)

        p = self.comments.get_one(lookup={'body': txt})
        _assertEqualComments(t, p)

    @pytest.mark.integration
    def test_delete(self):
        txt = 'DELETE ME'
        t = self.comments.create(fields={'text': txt})

        p = self.comments.get(lookup={'body': txt})
        length = len(p)
        self.assertTrue(length != 0)

        self.comments.delete(t)
        p = self.comments.get(lookup={'body': txt})
        self.assertTrue(len(p) < length)
