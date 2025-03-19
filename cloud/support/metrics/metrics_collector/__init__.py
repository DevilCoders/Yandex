#!/usr/bin/env python3
"""This module contains MetricsCollector class."""

import csv
import time
import logging

from startrek_client import Startrek
from metrics_collector.objects.issue import CloudSupportIssue, CloudIncIssue, UserFeedback
from metrics_collector.constants import BASE_API_URL, BASE_FILTER, USERAGENT
from metrics_collector.error import IssueInitError, DatabaseError

logger = logging.getLogger(__name__)
# logging.getLogger('yandex_tracker_client').setLevel(logging.WARNING)


class MetricsCollector:
    """This class provide interface for collect issues and upload them in database.

    Get startrek token here: https://oauth.yandex-team.ru/authorize?response_type=token&client_id=a7597fe0fbbf4cd896e64f795d532ad2

    Arguments:
      token: str – yandex oauth-token (required)
      useragent: str – useragent for startrek client
      base_url: str – startrek endpoint
      st_filter: str – filter for issues searching
      task_per_page: int - limit of issues per page

    Filter example: `Queue: CLOUDSUPPORT and Updated: >= now() - 1d`

    Method `run` collects a list of issues for a specified `st_filter`, converts and packages
    each issue from the list into objects, then loads them to the database.

    Method `print_as_tables` just prints a pretty table for each external comment.

    Method `export_to_csv` exports all external comments to a CSV file. Amazing, isn't it?

    """

    def __init__(self,
                 token=None,
                 db_client=None,
                 useragent=None,
                 base_url=None,
                 st_filter=None,
                 tasks_per_page=None):

        self.token = token
        self.useragent = useragent or USERAGENT
        self.base_url = base_url or BASE_API_URL
        self.st_filter = st_filter or BASE_FILTER
        self.tasks_per_page = tasks_per_page or 5000

        self.db_client = db_client
        self.client = Startrek(useragent=self.useragent,
                               base_url=self.base_url,
                               token=self.token)

    def _get_issue(self, issue_key):
        """Return issue as object."""
        return self.client.issues[issue_key]

    def _search_issues(self):
        """Find issues by class filter and return them as list of objects."""
        issues = self.client.issues.find(self.st_filter, per_page=self.tasks_per_page)
        logging.info(f'Found {len(issues)} issues.')
        return issues

    def _get_all_comments(self, issue_key):
        """Forced method for getting comments."""
        comments = self.client.issues[issue_key].comments.get_all()
        return comments

    def _ticket_handler(self, issue: object, upload=False):
        """Packages the cloudsupport issue into an object and loads it to the database."""

        if upload and self.db_client is None:
            raise DatabaseError('db_client required for upload flag')

        try:
            return CloudSupportIssue(
                startrek_key=issue.key,
                startrek_id=issue.id,
                external_id=issue.ticketID,
                external_author=issue.passportLogin,
                issue_author=issue.createdBy.id if issue.createdBy else None,
                type=issue.type.name,
                status=issue.status.name,
                pay=issue.pay,
                cloud_id=issue.cloudId,
                billing_id=issue.billingId,
                company_name=issue.companyName,
                partner=issue.partner,
                account_manager=issue.accountManager.id if issue.accountManager else None,
                environment=issue.environment,
                components=[i.name for i in issue.components],
                tags=issue.tags,
                comments=issue.comments,
                created_at=issue.createdAt,
                updated_at=issue.updatedAt,
                movedtoL2_at=issue.movedDate,
                priority=issue.priority.name,
                resolved_at=issue.resolvedAt,
                issue_type_changed=True if issue.changelog.get_all(field='type', type='IssueUpdated') else False,
                upload=upload,
                feedback=UserFeedback(
                    issue_key=issue.key,
                    general=issue.cloudFeedbackOne,
                    completely=issue.cloudFeedbackTwo,
                    rapid=issue.cloudFeedbackThree,
                    description=issue.cloudFeedbackFour,
                    created_at=issue.resolvedAt,
                    upload=upload,
                    db_client=self.db_client
                ),
                db_client=self.db_client)
        except Exception as err:
            raise IssueInitError(err)

    def _incident_handler(self, issue: object, upload=False):
        """Packages the cloudinc issue into an object and loads it to the database."""

        if upload and self.db_client is None:
            raise DatabaseError('db_client required for upload flag')

        try:
            return CloudIncIssue(
                startrek_key=issue.key,
                startrek_id=issue.id,
                assignee=issue.assignee.id if issue.assignee else None,
                issue_author=issue.createdBy.id if issue.createdBy else None,
                type=issue.type.name,
                priority=issue.priority.name,
                title=issue.summary,
                sla=0.0,
                sla_failed=False,
                components=[i.name for i in issue.components],
                tags=issue.tags,
                linked_issues=[],
                created_at=issue.createdAt,
                updated_at=issue.updatedAt,
                upload=upload,
                db_client=self.db_client)
        except Exception as err:
            raise IssueInitError(err)

    # FIXME: delete this func and make as table
    def print_incident(self, issue=None):
        issue = issue.upper()
        inc = self._incident_handler(self._get_issue(issue))
        print(inc.__dict__)

    def print_comments_as_table(self, issue=None):
        """Just prints a pretty table for each comment in the found issues.
        Shortcut for Issue().print_as_table()

        """
        if issue is not None:
            issue = issue.upper()
            return self._ticket_handler(self._get_issue(issue)).print_as_table()

        for issue in self._search_issues():
            self._ticket_handler(issue, upload=False).print_as_table()

    def export_comments_to_csv(self, path=None):
        """Exports all external comments to a CSV file."""
        timestamp = int(time.time())
        path = path or f'comments_{timestamp}.csv'
        fieldnames = [
            'issue', 'author', 'sla', 'raw_sla', 'sla_failed',
            'dev_wait_time', 'reopened', 'mistake_comment_type', 'issue_type_changed',
            'secondary_response', 'start_conversation_link', 'link', 'created_at'
        ]

        with open(path, 'w') as outfile:
            wr = csv.DictWriter(outfile, delimiter=',', fieldnames=fieldnames)
            wr.writeheader()
            for issue in self._search_issues():
                for i in self._ticket_handler(issue, upload=False).comments_handler():
                    wr.writerow({'issue': i.issue_key, 'author': i.author, 'sla': i.sla_as_str,
                                 'raw_sla': i.raw_sla, 'sla_failed': i.sla_failed,
                                 'dev_wait_time': i.dev_wait_time, 'reopened': i.reopened,
                                 'mistake_comment_type': i.mistake_comment_type,
                                 'issue_type_changed': i.issue_type_changed,
                                 'secondary_response': i.secondary_response,
                                 'start_conversation_link': i.start_conversation_link, 'link': i.link,
                                 'created_at': i.created_at})
        outfile.close()
        logger.info(f'Comments saved to "{path}"')

    def run(self):
        """Retrieves tasks and sends them to the database with preliminary preparation."""
        logging.info(f'Starting collect issues with filter: "{self.st_filter}"')

        issues = self._search_issues()
        counter = 1

        for issue in issues:
            state = round(counter / len(issues) * 100, 2)
            logging.info(f'Working with {issue.key}... [{state} %]')
            self._ticket_handler(issue, upload=True)
            counter += 1
