# -*- coding: utf-8 -*-
from functools import partial

import six
from six.moves import mock
import requests
import unittest
import pytest

from ids.exceptions import AuthError, IDSException, KeyIsAbsentError, BackendError

from ids.services.startrek.connector import StartrekConnector, BadResponseError


class TestStartrekConnector(unittest.TestCase):
    def setUp(self):
        unittest.TestCase.setUp(self)
        self.token = "c24ce44d0fc9459f85e3df9d8b79b40c"
        self.server = "https://st-api.test.yandex-team.ru"
        self.cfg = {
            'server': self.server,
            'oauth2_access_token': self.token,
            'timeout': 20,
        }

    @pytest.mark.integration
    def test_update_comment(self):
        sc = StartrekConnector(self.cfg)
        issue_key = "IDS-1"
        comment_text = ("ids.startrek.tests.TestStartrekConnector.test_update_c"
                        "omment\nhttps://github.yandex-team.ru/tools/ids/blob/m"
                        "aster/packages/startrek/tests/startrek_connector.py")
        new_comment_text = ("!!**UPDATED**!!: ids.startrek.tests.TestStartrekCo"
                            "nnector.test_update_comment\nhttps://github.yandex"
                            "-team.ru/tools/ids/blob/master/packages/startrek/t"
                            "ests/startrek_connector.py")

        comment = sc.create_comment(issue_key, {"text": comment_text})
        update = sc.update_comment(issue_key, comment["id"],
                                   {"text": new_comment_text})
        self.assertEquals(new_comment_text, update["text"])

    @pytest.mark.integration
    def test_create_comment(self):
        sc = StartrekConnector(self.cfg)
        comment_text = ("ids.startrek.tests.TestStartrekConnector.test_update_c"
                        "omment\nhttps://github.yandex-team.ru/tools/ids/blob/m"
                        "aster/packages/startrek/tests/startrek_connector.py")
        comment_created = sc.create_comment("IDS-1", {"text": comment_text})
        self.assertEqual(comment_text, comment_created["text"])

    @pytest.mark.integration
    def test_get_comments_one(self):
        sc = StartrekConnector(self.cfg)
        lookup = {"key": "IDS-1", "comment_id": "50ffeeb3e4b0ae7a9a3f1998"}
        comments = sc.search_comments(lookup)
        self.assertEquals(len([x for x in comments]), 1)

    @pytest.mark.integration
    def test_get_comments_list(self):
        sc = StartrekConnector(self.cfg)
        lookup = {"key": "IDS-1"}
        comments = sc.search_comments(lookup)
        self.assertTrue(len([x for x in comments]) > 0)

    @pytest.mark.integration
    def test_search_no_expand(self):
        sc = StartrekConnector(self.cfg)
        lookup = {"key": ["IDS-1", "IDS-2"]}
        issues = sc.search_issues(lookup=lookup, page=1)
        issue = six.next(issues)
        assert "comments" not in issue

    @pytest.mark.integration
    def test_search_expand(self):
        sc = StartrekConnector(self.cfg)
        lookup = {
            "key": ["IDS-1", "IDS-2"],
            "expand": ["comments"],
        }
        issues = sc.search_issues(lookup=lookup, page=1)
        issue = six.next(issues)
        assert "comments" in issue

    @pytest.mark.integration
    def test_input_params(self):
        self.assertRaises(AuthError, StartrekConnector,
                          {'server': 'http://localhost',
                           'oath2_access_token': None})
        self.assertRaises(AuthError, StartrekConnector,
                          {'server': 'http://localhost',
                           'oath2_access_token': ''})

    @pytest.mark.integration
    def test_search_returns_empty(self):
        sc = StartrekConnector(self.cfg)
        issues = sc.search_issues(lookup={"key": "IDS_TESTS-0"})
        self.assertRaises(BadResponseError, partial(next, issues))

    @pytest.mark.integration
    def test_get_one_issue_type(self):
        sc = StartrekConnector(self.cfg)
        id = "4f6b271330049b12e72e3ef3"
        self.assertEquals(id, sc.get_one_issue_type(id)["id"])

    @pytest.mark.integration
    def test_get_search_issue_types(self):
        sc = StartrekConnector(self.cfg)
        lookup = {"id": "4f6b271330049b12e72e3ef3"}
        issue_type = six.next(sc.search_issue_types(lookup))
        self.assertEquals("4f6b271330049b12e72e3ef3", issue_type["id"])

    @pytest.mark.integration
    def test_get_one_issue_auth_raises(self):
        opts = self.cfg
        opts["oauth2_access_token"] = "ids-integration-test-token"
        sc = StartrekConnector(opts)
        self.assertRaises(AuthError, sc.get_one_issue, "IDS-1")

    @pytest.mark.integration
    def test_create_ok(self):
        sc = StartrekConnector(self.cfg)
        basic_fields = {
            'queue': {'id': '50ddeae0e4b0c2036ae8c24a'},
            'summary': 'IDS integration tests ISSUE',
            'description': 'ids.startrek.tests.TestStartrekConnector.test_creat'
                           'e_ok\nhttps://github.yandex-team.ru/tools/ids/blob/'
                           'master/packages/startrek/tests/startrek_connector.p'
                           'y'
        }
        sc.create_issue(basic_fields)

    @pytest.mark.integration
    def test_create_not_found(self):
        sc = StartrekConnector(self.cfg)
        absent_queue_id = "1ff3f23ce4b0e2ac27f6eb95"
        basic_fields = {
            'queue': {'id': absent_queue_id},
            'summary': 'IDS integration tests ISSUE',
            'description': 'ids.startrek.tests.TestStartrekConnector.test_creat'
                           'e_not_found\nhttps://github.yandex-team.ru/tools/id'
                           's/blob/master/packages/startrek/tests/startrek_conn'
                           'ector.py'
        }
        self.assertRaises(KeyIsAbsentError, sc.create_issue, basic_fields)

    @pytest.mark.integration
    def test_create_500(self):
        sc = StartrekConnector(self.cfg)
        bad_queue_id = "ids-integration-tests-token"
        basic_fields = {
            'queue': {'id': bad_queue_id},
            'summary': 'IDS integration tests ISSUE',
            'description': 'ids.startrek.tests.TestStartrekConnector.test_creat'
                           'e_500\nhttps://github.yandex-team.ru/tools/ids/blob'
                           '/master/packages/startrek/tests/startrek_connector.'
                           'py'
        }
        self.assertRaises(IDSException, sc.create_issue, basic_fields)

    @pytest.mark.integration
    def test_service_unreachable_get(self):
        cfg = self.cfg.copy()
        cfg['server'] = 'http://localhost'
        sc = StartrekConnector(cfg)

        class MockSession(mock.Mock):
            def get(self, *args, **kwargs):
                raise requests.exceptions.Timeout()

        sc.http = MockSession()
        self.assertRaises(BackendError, sc.get_one_issue, "IDS-1")

    @pytest.mark.integration
    def test_service_unreachable_create(self):
        cfg = self.cfg.copy()
        cfg['server'] = 'http://localhost'
        sc = StartrekConnector(cfg)

        class MockSession(mock.Mock):
            def post(self, *args, **kwargs):
                raise requests.exceptions.Timeout()

        sc.http = MockSession()
        bad_queue_id = "ids-integration-tests-token"
        basic_fields = {
            'queue': {'id': bad_queue_id},
            'summary': 'IDS integration tests ISSUE',
            'description': 'ids.startrek.tests.TestStartrekConnector.test_servi'
                           'ce_unreachable_create\nhttps://github.yandex-team.r'
                           'u/tools/ids/blob/master/packages/startrek/tests/sta'
                           'rtrek_connector.py'
        }
        self.assertRaises(BackendError, sc.create_issue, basic_fields)
