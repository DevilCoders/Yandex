#!/usr/bin/env python3
"""This module contains StartrekClient class."""

import logging

from startrek_client import Startrek
from core.objects.base import Base
from core.constants import USERAGENT, STARTREK_BASE_FILTER
from utils.config import Config

logger = logging.getLogger(__name__)


class StartrekClient(Base):
    """This class provide interface for work with Startrek.

    If args not specified – using from config or default values.

    Arguments:
      token: str – yandex oauth-token (required from config or arg)
      useragent: str – useragent for startrek client
      base_url: str – startrek endpoint
      st_filter: str – filter for issues searching
      task_per_page: int - limit of issues per page

    Filter example: `Queue: CLOUDSUPPORT and Updated: >= now() - 1d`

    """

    def __init__(self,
                 token=None,
                 useragent=None,
                 base_url=None,
                 st_filter=None,
                 tasks_per_page=None):

        self.token = token or Config.STARTREK_TOKEN
        self.useragent = useragent or USERAGENT
        self.base_url = base_url or Config.STARTREK_ENDPOINT
        self.st_filter = st_filter or STARTREK_BASE_FILTER
        self.tasks_per_page = tasks_per_page or 5000

        self.client = Startrek(
            useragent=self.useragent,
            base_url=self.base_url,
            token=self.token
        )

    def raw_client(self):
        """Return raw Startrek client."""
        return self.client

    def get_issue(self, issue_key: str):
        """Return issue as object."""
        return self.client.issues[issue_key]

    def search_issues(self, issue_filter=None):
        """Find issues by class filter and return them as list of objects."""
        issue_filter = issue_filter or self.st_filter
        issues = self.client.issues.find(issue_filter, per_page=self.tasks_per_page)
        logging.info(f'Found {len(issues)} issues by filter: {issue_filter}')
        return issues

    def get_all_comments(self, issue_key: str):
        """Forced method for getting comments."""
        comments = self.client.issues[issue_key].comments.get_all()
        return comments

    def get_comments_for_author(self, issue_key: str, login: str):
        """Return comments in ticket for author."""
        comments = self.client.issues[issue_key].comments.get_all()
        links = []
        for comment in comments:
            value = comment._value
            user = str(value.get("createdBy")).split("\'")
            if user[1] == login and value.get("type") == 'outgoing':
                links.append(f'https://st.yandex-team.ru/{issue_key}#{value.get("longId")}')
        return links