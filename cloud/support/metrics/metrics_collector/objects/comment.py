#!/usr/bin/env python3
"""This module contains Comment class."""

import logging

from metrics_collector.objects.base import Base
from metrics_collector.constants import BASE_URL
from metrics_collector.helpers import string_delta_time, make_datetime

logger = logging.getLogger(__name__)


class ExternalComment(Base):
    """This object repesents an outgoing comment.

    Arguments:
      startrek_id: str
      start_conversation_id: str
      issue_key: str
      start_conversation_index: int
      comment_index: int
      author: str
      sla: float
      raw_sla: float
      sla_failed: bool
      dev_wait_time: float
      secondary_response: bool
      reopened: bool
      mistake_comment_type: bool
      link: str
      start_conversation_link: str
      created_at: str (can be converted to datetime)
      upload: bool
      db_client: object

    """

    def __init__(self,
                 startrek_id=None,
                 start_conversation_id=None,
                 issue_key=None,
                 start_conversation_index=None,
                 comment_index=None,
                 author=None,
                 sla=None,
                 raw_sla=None,
                 sla_failed=None,
                 dev_wait_time=None,
                 secondary_response=None,
                 reopened=None,
                 mistake_comment_type=None,
                 issue_type_changed=None,
                 link=None,
                 start_conversation_link=None,
                 created_at=None,
                 upload=False,
                 db_client=None,
                 **kwargs):

        self.startrek_id = startrek_id
        self.start_conversation_id = start_conversation_id
        self.issue_key = issue_key
        self.start_conversation_index = start_conversation_index
        self.comment_index = comment_index
        self.author = author
        self.sla = sla
        self.raw_sla = raw_sla
        self.sla_as_str = string_delta_time(int(self.sla * 60)) if isinstance(self.sla, (float, int)) else str(self.sla)
        self.sla_failed = sla_failed
        self.dev_wait_time = dev_wait_time
        self.secondary_response = secondary_response
        self.reopened = reopened
        self.mistake_comment_type = mistake_comment_type
        self.issue_type_changed = issue_type_changed
        self.link = f'{BASE_URL}/{issue_key}#{startrek_id}'
        self.start_conversation_link = f'{BASE_URL}/{issue_key}#{start_conversation_id}'
        self.created_at = make_datetime(created_at) if isinstance(created_at, str) else created_at

        self._db_session = db_client
        self._upload = upload
        self._upload_to_database()

    @property
    def dev_time_as_str(self):
        """Waiting time for a response from developers as human readable string."""
        if isinstance(self.dev_wait_time, (float, int)):
            seconds = int(self.dev_wait_time) * 60
            return string_delta_time(seconds=seconds)
        return self.dev_wait_time

    @property
    def _is_exist(self):
        if not self._upload:
            return
        return self._db_session.get_comment(self)

    def _upload_to_database(self):
        if not self._upload:
            return

        if not self._is_exist:
            self._db_session.add_comment(self)
        else:  # comment fields comparison and updating
            try:
                assert self._is_exist == self.to_dict()
            except AssertionError:
                self._db_session.update_comment(self, self._is_exist)


class IncomingComment(Base):
    """This object repesents an incoming comment from external user.

    Arguments:
      startrek_id: str
      issue_key: str
      created_at: str (can be converted to datetime)
      upload: bool
      db_client: object

    """

    def __init__(self,
                 startrek_id=None,
                 issue_key=None,
                 author=None,
                 created_at=None,
                 upload=False,
                 db_client=None,
                 **kwargs):

        self.issue_key = issue_key
        self.startrek_id = startrek_id
        self.author = author
        self.created_at = make_datetime(created_at) if isinstance(created_at, str) else created_at

        self._upload = upload
        self._db_session = db_client
        self._upload_to_database()

    @property
    def _is_exist(self):
        if not self._upload:
            return
        return self._db_session.get_incoming_msg(self)

    def _upload_to_database(self):
        if not self._upload:
            return

        if not self._is_exist:
            self._db_session.add_incoming_msg(self)
        else:  # comment fields comparison and updating
            try:
                assert self._is_exist == self.to_dict()
            except AssertionError:
                self._db_session.update_incoming_msg(self, self._is_exist)
