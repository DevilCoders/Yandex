# -*- coding: utf-8 -*-

import unittest
import pytest

from ids.registry import registry

class TestStartrekIssueEventsBoundRepository(unittest.TestCase):
    def setUp(self):
        self.token = 'c24ce44d0fc9459f85e3df9d8b79b40c'
        self.link = 'https://st-api.test.yandex-team.ru'
        self.rep = registry.get_repository('startrek', 'issues',
                            user_agent='ids-test',
                            oauth2_access_token=self.token,
                            server=self.link)

        self.parent = self.rep.get_one(lookup={'key': 'IDS-1'})
        self.issue_events = self.parent['rep_issue_events']

        self.startrek = self.rep.STARTREK_CONNECTOR_CLASS(self.rep.options)

    @pytest.mark.integration
    def test_getting_from_parent_resource(self):
        lookup = {"event_id": "51a36de5e4b078f099441db0"}
        event = self.issue_events.get(lookup=lookup)[0]
        self.assertEquals(u"IssueCommentUpdated", event["eventName"])
