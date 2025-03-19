#!/usr/bin/env python3
"""This module contains CloudSupportIssue, CloudIncIssue, UserFeedback, Tag and Component classes."""

import re
import logging

from datetime import datetime

from metrics_collector.objects.base import Base
from metrics_collector.objects.comment import ExternalComment, IncomingComment
from metrics_collector.error import MetricsCollectorError
from metrics_collector.helpers import get_delta_time, string_delta_time, make_datetime, demojify
from metrics_collector.constants import (SLA, REOPEN_ROBOTS, SYSTEM_ROBOTS, ISSUE_TYPES,
                                         SUPPORTS, CLOSED, FIRST_COMMENT_ROBOTS)

logger = logging.getLogger(__name__)


class CloudSupportIssue(Base):
    """This object repesents a issue from CLOUDSUPPORT queue.

    Arguments:
      startrek_key: str
      startrek_id: str
      external_id: str
      external_author: str
      issue_author: str
      type: str
      status: str
      pay: str
      cloud_id: str
      billing_id: str
      company_name: str
      partner: str
      account_manager: str
      environment: str
      components: list
      tags: list
      comments: list
      created_at: str (can be converted to datetime)
      updated_at: str (can be converted to datetime)
      movedtoL2_at: str (can be converted to datetime)
      priority: str
      resolved_at: str (can be converted to datetime)
      upload: bool
      db_client: object

    """

    def __init__(self,
                 startrek_key=None,
                 startrek_id=None,
                 external_id=None,
                 external_author=None,
                 issue_author=None,
                 type=None,
                 status=None,
                 pay=None,
                 cloud_id=None,
                 billing_id=None,
                 company_name=None,
                 partner=None,
                 account_manager=None,
                 environment=None,
                 issue_type_changed=None,
                 components=None,
                 tags=None,
                 comments=None,
                 created_at=None,
                 updated_at=None,
                 movedtoL2_at=None,
                 priority=None,
                 resolved_at=None,
                 feedback=None,
                 upload=False,
                 db_client=None,
                 **kwargs):

        self.startrek_key = startrek_key
        self.startrek_id = startrek_id
        self.external_id = external_id
        self.external_author = external_author
        self.issue_author = issue_author
        self.type = str(type).lower() if type in ISSUE_TYPES else 'question'
        self.status = status.lower() if isinstance(status, str) else status
        self.pay = str(pay).lower() if pay in SLA else 'free'
        self.cloud_id = cloud_id
        self.billing_id = billing_id
        self.company_name = company_name.replace('"', '') if isinstance(company_name, str) else company_name
        self.partner = partner or None
        self.account_manager = account_manager
        self.managed = False if self.account_manager is None else True
        self.environment = environment
        self.issue_type_changed = issue_type_changed
        self.feedback_received = True if isinstance(feedback.general, int) and feedback.general > 0 else False
        self._components = components or []
        self._tags = tags or []
        self._comments = list(comments) or []
        self._created_at = created_at  # raw string for comments_handler
        self._feedback = feedback
        self.created_at = make_datetime(created_at) if isinstance(created_at, str) else created_at
        self.updated_at = make_datetime(updated_at) if isinstance(updated_at, str) else updated_at
        self.movedtoL2_at = make_datetime(movedtoL2_at) if isinstance(movedtoL2_at, str) else movedtoL2_at
        self.priority = priority
        self.resolved_at = make_datetime(resolved_at) if isinstance(resolved_at, str) else resolved_at

        self._db_session = db_client
        self._upload = upload
        self._upload_to_database()

    @property
    def _is_exist(self):
        """Return data as dict if exist, else return False."""
        if not self._upload:
            return
        return self._db_session.get_issue(self)

    def _upload_to_database(self):
        """Prepare, validate, and upload the issue along with its
        dependent components to the database.

        """
        if self._db_session is None and self._upload:
            raise MetricsCollectorError('db_client required for upload flag')

        # handle read-only mode and stop ignoring tracker support tickets
        if not self._upload:
            return

        if not self._is_exist:
            self._db_session.add_issue(self)
        else:  # issue fields comparison and updating
            try:
                assert self._is_exist == self.to_dict()
            except AssertionError:
                self._db_session.update_issue(self, self._is_exist)

        # handling feedback from external users
        if self.resolved_at is not None:
            self._feedback._upload_to_database()

        # handling issue components
        for component in self._components:
            Component(name=component, relationship=self.startrek_key,
                db_client=self._db_session, upload=self._upload)

        # handling issue tags
        for tag in self._tags:
            Tag(name=tag, relationship=self.startrek_key,
                db_client=self._db_session, upload=self._upload)

        # handling ticket head as incoming message from external user
        IncomingComment(
            startrek_id=f'_{self.startrek_id}',
            issue_key=self.startrek_key,
            author=self.external_author,
            created_at=self.created_at,
            upload=self._upload,
            db_client=self._db_session
        )

        # handling comments and authors
        self.comments_handler()

    # TODO: colors for FAILS and Support Plans
    def print_as_table(self):
        """Just prints a pretty table for each comment in the found issue."""
        from prettytable import PrettyTable

        sla = string_delta_time(SLA[self.pay][self.type] * 60)
        table = PrettyTable()
        table.header = True
        # table.title not work with arcadia prettytable
        # table.title = f'{self.startrek_key} | {self.type} | {self.pay} | SLA threshold: {sla}'
        title = f'{self.startrek_key} | {self.type} | {self.pay} | SLA threshold: {sla}'
        table.field_names = [
            'AUTHOR', 'SLA', 'FAILED?', 'AWAIT DEV', 'SECONDARY?', 'REOPEN?', 'MISSED?',
            'ISSUE TYPE CHANGED?', 'START CONVERSATION LINK', 'EXTERNAL OUT COMMENT LINK'
        ]
        table.align = 'l'

        for i in self.comments_handler():
            table.add_row([i.author, i.sla_as_str, i.sla_failed, i.dev_time_as_str,
                           i.secondary_response, i.reopened, i.mistake_comment_type,
                           i.issue_type_changed, i.start_conversation_link, i.link])

        print(title, table, sep='\n')

    def comments_handler(self):
        """A mediocre algorithm for collecting useful information in a conversation,
        converting and packaging each external response from a list into an object
        and uploading it to the database.

        The conversation always starts with an incoming comment from the user and ends with
        an outgoing response from Support.

        The SLA is counted from the first incoming message from the user in the conversation,
        regardless of the number of nested incoming messages in the conversation to the outgoing
        Support response.

        If this is the second consecutive response from Support, the SLA is counted from the
        previous outgoing Support response.

        To debug the SLA conversation window, you can view the conversation start index
        and the support response index.

        """
        logging.info(f'Handling comments for {self.startrek_key}')

        external_comments = list()
        reopened = False
        start_conv_index = 0
        start_conv_id = 0
        prev_index_out = 0  # outgoing comment streak. (for secondary outgoing flag)
        cursor = 0  # incoming cursor
        sla = self._created_at  # replaced by incoming created_at
        wait_time = 0  # dev wait time temp storage

        # checking whether the first comment is a system comment
        start_pos = 1 if self._comments and self._comments[0].createdBy.id in FIRST_COMMENT_ROBOTS else 0

        # calculate SLA algorithm
        for i in range(start_pos, len(self._comments)):
            comment = self._comments[i]

            # bypass comments without types from old issues
            if not hasattr(comment, 'type'):
                logger.warning(f"Can't calculate the SLA for {self.startrek_key} comments, because comments " + \
                                "don't have the 'type' attribute.")
                return external_comments

            # first item for start conversation and calculate external sla
            if comment.type == 'incoming':
                if cursor == 0 and sla == 0:
                    sla = comment.createdAt
                    start_conv_index = i
                    start_conv_id = comment.longId
                cursor += 1
                reopened = False
                prev_index_out = 0  # clean outgoing comment streak

                # handle incoming message from external user
                IncomingComment(
                    startrek_id=comment.longId,
                    issue_key=self.startrek_key,
                    author=self.external_author,
                    created_at=comment.createdAt,
                    upload=self._upload,
                    db_client=self._db_session
                )

            # response from Support team. End of conversation for calculate SLA
            elif comment.type == 'outgoing' and comment.createdBy.id in SUPPORTS:
                secondary_response = False  # True if out+out without internal comments in the middle
                mistake = True if comment.transport != 'email' and self.environment is None else False

                if sla == 0:  # fixed, second_out calculate bad sla, see code comment below
                    cmt_sla = 0.0 # get_delta_time(self._created_at, comment.createdAt) if not reopened else 0
                elif self.issue_author not in SYSTEM_ROBOTS:
                    cmt_sla = 0.0
                else:
                    cmt_sla = get_delta_time(sla, comment.createdAt)

                sla_fail = True if cmt_sla > SLA[self.pay][self.type] else False

                # predicting the response time of the developers
                if prev_index_out != 0:
                    start_conv_id = self._comments[i - 1].longId
                    start_conv_index = i - 1
                    secondary_response = True if not reopened else False
                    wait_time = get_delta_time(self._comments[prev_index_out].createdAt, comment.createdAt)
                    # cmt_sla = wait_time  # for second response from Support, delta(prev, curr)

                # generate perfect comment for stats
                cmt = ExternalComment(
                    startrek_id=comment.longId,
                    issue_key=self.startrek_key,
                    secondary_response=secondary_response,
                    author=comment.createdBy.id,
                    reopened=reopened,
                    dev_wait_time=float(wait_time) if not reopened else 0.0,
                    sla=cmt_sla if not sla_fail else (SLA[self.pay][self.type] - cmt_sla),
                    raw_sla=cmt_sla,
                    created_at=comment.createdAt,
                    sla_failed=sla_fail,
                    mistake_comment_type=mistake,
                    comment_index=i,
                    start_conversation_id=str(start_conv_id),
                    start_conversation_index=start_conv_index,
                    issue_type_changed=self.issue_type_changed,
                    upload=self._upload,
                    db_client=self._db_session
                )

                # store external comment and clean temp data
                external_comments.append(cmt)
                prev_index_out = i
                start_conv_index, start_conv_id, sla, wait_time, cursor = 0, 0, 0, 0, 0
                reopened = False

            # dev/robot internal comments
            else:
                if comment.createdBy.id in SYSTEM_ROBOTS:
                    if re.search(CLOSED, comment.text):
                        start_conv_index, start_conv_id, sla, wait_time, cursor = 0, 0, 0, 0, 0
                if comment.createdBy.id in REOPEN_ROBOTS:
                    reopened = True
                if cursor != 0 and comment.createdBy.id not in SUPPORTS:
                    wait_time = get_delta_time(sla, comment.createdAt)

        return external_comments


class Component(Base):
    """This object represents a component of the startrek issue.

    Arguments:
      name: str
      relationship: str (issue.startrek_key)
      db_client: object
      relationship_table: str
      upload: bool

    """

    def __init__(self,
                 name=None,
                 relationship=None,
                 db_client=None,
                 relationship_table=None,
                 upload=False):

        self.name = name
        self.relationship = relationship

        self._relationship_table = relationship_table or 'issue_components'
        self._db_session = db_client
        self._upload = upload
        self._upload_to_database()

    @property
    def _is_exist(self):
        """Return data as dict if exist, else False."""
        if not self._upload:
            return
        return self._db_session.get_component(self)

    @property
    def _relationship_is_exist(self):
        """Return data as dict if exist, else False."""
        if not self._upload:
            return
        return self._db_session.check_component(self, table=self._relationship_table)

    def _upload_to_database(self):
        if not self._upload:
            return

        if not self._is_exist:
            self._db_session.add_component(self)

        if not self._relationship_is_exist:
            self._db_session.set_component(self, table=self._relationship_table)


class Tag(Base):
    """This object represents a tag of the startrek issue.

    Arguments:
      name: str
      relationship: str (issue.startrek_key)
      db_client: object
      relationship_table: str
      upload: bool

    """

    def __init__(self,
                 name=None,
                 relationship=None,
                 db_client=None,
                 relationship_table=None,
                 upload=False):

        self.name = name
        self.relationship = relationship

        self._relationship_table = relationship_table or 'issue_tags'
        self._db_session = db_client
        self._upload = upload
        self._upload_to_database()

    @property
    def _is_exist(self):
        """Return data as dict if exist, else False."""
        if not self._upload:
            return
        return self._db_session.get_tag(self)

    @property
    def _relationship_is_exist(self):
        """Return data as dict if exist, else False."""
        if not self._upload:
            return
        return self._db_session.check_tag(self, table=self._relationship_table)

    def _upload_to_database(self):
        if not self._upload:
            return

        if not self._is_exist:
            self._db_session.add_tag(self)

        if not self._relationship_is_exist:
            self._db_session.set_tag(self, table=self._relationship_table)


class UserFeedback(Base):
    """This object represents feedback from users.

    Arguments:
      issue_key: str
      general: int
      completely: int
      rapid: int
      description: str
      created_at: str (can be converted to datetime)
      upload: bool
      db_client: object

    """

    def __init__(self,
                 issue_key=None,
                 general=None,
                 completely=None,
                 rapid=None,
                 description=None,
                 created_at=None,
                 upload=False,
                 db_client=None,
                 **kwargs):

        self.issue_key = issue_key
        self.general = int(general) if general else 0
        self.completely = int(completely) if completely else None
        self.rapid = int(rapid) if rapid else None
        self.description = demojify(description).replace('"', '`') if isinstance(description, str) else None
        self.created_at = make_datetime(created_at) if isinstance(created_at, str) else created_at

        self._upload = upload
        self._db_session = db_client
        self._normalize()

    def _normalize(self):
        """The normalisation for the old feedback and empty values.

        If the secondary score is empty, it will take the value of the overall score.
        Previously, when setting an overall rating of 5 stars, the other parameters were empty;
        now the others take the value from the overall rating.

        """
        if self.general > 0 and self.rapid is None:
            self.rapid = self.general

        if self.general > 0 and self.completely is None:
            self.completely = self.general

    def _validate(self):
        """General score cannot be zero or None."""
        if self.general is None or self.general == 0:
            return

        if self.created_at is None:
            return

        return True

    @property
    def _is_exist(self):
        """Return data as dict if exist, else False."""
        if not self._upload:
            return

        if self._validate():
            return self._db_session.get_feedback(self)
        return

    # Run from issue
    def _upload_to_database(self):
        if not self._upload:
            return

        if not self._validate():
            return

        if not self._is_exist:
            self._db_session.add_feedback(self)
        else:  # feedback fields comparison and updating
            try:
                assert self._is_exist == self.to_dict()
            except AssertionError:
                self._db_session.update_feedback(self, self._is_exist)


class CloudIncIssue(Base):
    """This object repesents a issue from CLOUDINC queue.

    Arguments:
      startrek_key: str
      startrek_id: str
      assignee: str
      issue_author: str
      type: str
      priority: str
      title: str
      sla: float
      sla_failed: bool
      components: list
      tags: list
      linked_issues: list
      created_at: str (can be converted to datetime)
      updated_at: str (can be converted to datetime)
      upload: bool
      db_client: object

    """

    def __init__(self,
                 startrek_key=None,
                 startrek_id=None,
                 assignee=None,
                 issue_author=None,
                 type=None,
                 priority=None,
                 title=None,
                 sla=None,
                 sla_failed=None,
                 components=None,
                 tags=None,
                 linked_issues=None,
                 created_at=None,
                 updated_at=None,
                 upload=False,
                 db_client=None,
                 **kwargs):

        self.startrek_key = startrek_key
        self.startrek_id = startrek_id
        self.assignee = assignee
        self.issue_author = issue_author
        self.type = str(type).lower() if type in ISSUE_TYPES else 'task'
        self.priority = priority
        self.title = title
        self.sla = sla
        self.sla_failed = sla_failed
        self._components = components or []
        self._tags = tags or []
        self._linked_issues = linked_issues or []  # handling issues in CloudSupportIssue with upload flag
        self._created_at = created_at  # raw string for comments_handler
        self.created_at = make_datetime(created_at) if isinstance(created_at, str) else created_at
        self.updated_at = make_datetime(updated_at) if isinstance(updated_at, str) else updated_at

        self._upload = upload
        self._db_session = db_client
        self._upload_to_database()

    @property
    def _is_exist(self):
        """Return data as dict if exist, else return False."""
        if not self._upload:
            return
        # FIXME: new table
        pass  # return self._db_session.get_issue(self)

    def _upload_to_database(self):
        """Prepare, validate, and upload the issue along with its
        dependent components to the database.

        """
        if self._db_session is None and self._upload:
            raise MetricsCollectorError('db_client required for upload flag')

        if not self._upload:
            return

        if not self._is_exist:
            # FIXME: new table
            pass # self._db_session.add_issue(self)
        else:  # issue fields comparison and updating
            try:
                pass  # assert self._is_exist == self.to_dict()
            except AssertionError:
                # FIXME: new table
                pass  # self._db_session.update_issue(self, self._is_exist)

        # FIXME: new table
        # handling components
        # for component in self._components:
        #     Component(name=component, relationship=self.startrek_key,
        #         db_client=self._db_session, upload=self._upload)

        # FIXME: new table
        # handling tags
        # for tag in self._tags:
        #     Tag(name=tag, relationship=self.startrek_key,
        #         db_client=self._db_session, upload=self._upload)
