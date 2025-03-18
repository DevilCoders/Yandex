# -*- coding: utf-8 -*-

import unittest
import pytest

from requests.exceptions import MissingSchema

from ids.exceptions import AuthError
from ids.services.jira.connector import JiraConnector


class TestJiraConnector(unittest.TestCase):

    @pytest.mark.integration
    def test_input_params(self):
        self.assertRaises(AuthError, JiraConnector, None, None)
        self.assertRaises(AuthError, JiraConnector, None, '')
        self.assertRaises(AuthError, JiraConnector, '', None)

        jc = JiraConnector('server', 'token')
        self.assertRaises(MissingSchema, jc.connection.search_issues, '')

    @pytest.mark.integration
    def test_getting_issues(self):
        jc = JiraConnector('https://jira.test.tools.yandex-team.ru',
                           '5431b13a8cca4cc5b8b4cce46929d3aa')

        count = 3
        L = map(lambda x: 'PLAN-{0}'.format(x), range(100, 100 + count))
        r = list(jc.get_all_issues('key in (' + ', '.join(L) + ')', chunk_size=1))
        self.assertTrue(len(r) == count)
