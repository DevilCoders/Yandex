# -*- coding: utf-8 -*-

from ids.services.jira.connector import JiraConnector


class FakeTicket(object):
    # TODO
    pass


class FakeComment(object):
    # TODO
    def __init__(self):
        self.raw = {'author': {'name': 'mixael', 'active': True}}

    def update(self, **kws):
        # TODO
        pass

    def delete(self):
        # TODO
        pass


class JiraFakeConnector(JiraConnector):
    def __init__(self, server, access_token):
        self._check_auth_params(server, access_token)

        self.connection = self

    def create_issue(fields=None):
        if fields is None:
            fields = {}

        # TODO

    def search_issues(self, query, startAt=0, maxResults=50, fields=None):
        if fields is None:
            fields = {}

        # TODO

    def add_comment(issue_key=None, body=''):
        if issue_key is None:
            raise ValueError()

        # TODO

    def comments(issue_key=None):
        if issue_key is None:
            raise ValueError()

        # TODO
