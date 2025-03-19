#!/usr/bin/env python3
"""This module contains Database class."""

import logging
import pymysql
from pymysql.cursors import DictCursor
from pymysql.err import OperationalError

from metrics_collector.constants import BOOLEAN_KEYS
from metrics_collector.helpers import make_perfect_dict, retry

logger = logging.getLogger(__name__)


class Database:
    """This class provides an interface for working with a MySQL database.

    Arguments:
      host: str
      port: int
      db_name: str
      user: str
      passwd: str
      ssl: str - absolute path to CA cert .crt, like a /root/.certs/root.crt
      charset: str - like a 'utf8'

    CA URL: https://storage.yandexcloud.net/cloud-certs/CA.pem

    """

    def __init__(self,
                 host=None,
                 port=None,
                 db_name=None,
                 user=None,
                 passwd=None,
                 ssl=None,
                 use_unicode='true',
                 charset='utf8mb4'):

        self.host = host
        self.port = port
        self.db_name = db_name
        self.user = user
        self.passwd = passwd
        self.charset = charset
        self.use_unicode = use_unicode or 'true'
        self.cusorclass = DictCursor
        self.ssl = ssl

        self.ca = {
            'ca': self.ssl
        }

    @retry(OperationalError)
    def connection(self):
        conn = pymysql.connect(
            host=self.host,
            user=self.user,
            db=self.db_name,
            passwd=self.passwd,
            ssl=self.ca,
            cursorclass=self.cusorclass,
            use_unicode=self.use_unicode,
            charset=self.charset
        )
        return conn

    def query(self, sql, fetchall=False, close=False):
        """A simple method for executing a DB query.

        Args:
          sql: str - sql query.
          fetchall: bool - return many results.
          close: bool - if true, connect will be closed after execute.

        """
        connection = self.connection()
        logger.debug(connection, sql)
        result = None

        try:
            connection.ping(reconnect=True)
            with connection.cursor() as cursor:
                cursor.execute(sql)
                result = cursor.fetchall() if fetchall else cursor.fetchone()
                logger.debug(f'MySQL.result: {result}')
            cursor.close()  # TESTING, DELETE IF THERE ARE PROBLEMS
        except Exception as err:
            logger.error(f'MySQL.query: {sql}, Error: {err}')
        else:
            connection.commit()
        finally:
            connection.close() if close else None
            return result

    # ISSUES

    def add_issue(self, issue: object, table='issues'):
        """Add issue in to database."""
        # resolved_at is NULL by default and be changed on update issue
        sql = f'INSERT INTO `{table}` ' + \
              f'(startrek_key, startrek_id, external_id, external_author, issue_author, type, status, pay, cloud_id, ' + \
              f'billing_id, company_name, partner, account_manager, managed, environment, issue_type_changed, ' + \
              f'feedback_received, created_at, updated_at, movedtoL2_at, priority) ' + \
              f'VALUES ("{issue.startrek_key}", "{issue.startrek_id}", "{issue.external_id}", "{issue.external_author}", ' + \
              f'"{issue.issue_author}", "{issue.type}", "{issue.status}", "{issue.pay}", "{issue.cloud_id}", ' + \
              f'"{issue.billing_id}", "{issue.company_name}", "{issue.partner}", "{issue.account_manager}", ' + \
              f'{issue.managed}, "{issue.environment}", {issue.issue_type_changed}, {issue.feedback_received}, ' + \
              f'"{issue.created_at}", "{issue.updated_at}", "{issue.movedtoL2_at}", "{issue.priority}")'

        logger.info(f'Trying to add issue {issue.startrek_key} to DB (table: {table}).')
        return self.query(sql, close=True)

    def get_issue(self, issue: object, table='issues'):
        """Returns record as dict, if it exists."""
        sql = f'SELECT * ' + \
              f'FROM `{table}` ' + \
              f'WHERE startrek_key = "{issue.startrek_key}"'

        result = self.query(sql, close=True)
        if not result:
            logging.debug(f'Issue {issue.startrek_key} not found in DB.')
            return False
        return make_perfect_dict(result)

    def update_issue(self, issue: object, old_data: dict, table='issues'):
        """Updating only different values for issue in the database."""
        for key, value in issue.to_dict().items():
            if key == 'resolved_at' and value is None:
                continue
            if value != old_data[key]:
                logging.info(f'Updating "{key}" for {issue.startrek_key} in DB. Old: {old_data[key]}, New: {value}')
                new_value = value if key in BOOLEAN_KEYS else f'"{value}"'
                sql = f'UPDATE `{table}` ' + \
                      f'SET {key} = {new_value} ' + \
                      f'WHERE startrek_key = "{issue.startrek_key}"'

                self.query(sql, close=True)

    def delete_issue(self, issue: object, table='issues'):
        pass

    # EXTERNAL COMMENTS

    def add_comment(self, comment: object, table='comments'):
        """Add comment in to database."""
        sql = f'INSERT INTO `{table}` ' + \
              f'(startrek_id, start_conversation_id, issue_key, start_conversation_index, ' + \
              f'comment_index, author, sla, raw_sla, sla_as_str, sla_failed, dev_wait_time, ' + \
              f'secondary_response, reopened, mistake_comment_type, issue_type_changed, ' + \
              f'link, start_conversation_link, created_at) ' + \
              f'VALUES ("{comment.startrek_id}", "{comment.start_conversation_id}", "{comment.issue_key}", ' + \
              f'{comment.start_conversation_index}, {comment.comment_index}, "{comment.author}", {comment.sla}, ' + \
              f'{comment.raw_sla}, "{comment.sla_as_str}", {comment.sla_failed}, {comment.dev_wait_time}, ' + \
              f'{comment.secondary_response}, {comment.reopened}, {comment.mistake_comment_type}, ' + \
              f'{comment.issue_type_changed}, "{comment.link}", "{comment.start_conversation_link}", ' + \
              f'"{comment.created_at}")'

        logger.info(f'Trying to add comment {comment.startrek_id} to DB.')
        return self.query(sql, close=True)

    def get_comment(self, comment: object, table='comments'):
        """Returns record as dict, if it exists."""
        sql = f'SELECT * ' + \
              f'FROM `{table}` ' + \
              f'WHERE startrek_id = "{comment.startrek_id}"'

        result = self.query(sql, close=True)
        if not result:
            logging.debug(f'Comment {comment.startrek_id} not found in DB.')
            return False
        return make_perfect_dict(result)

    def update_comment(self, comment: object, old_data: dict, table='comments'):
        """Updating only different values for comment in database."""
        for key, value in comment.to_dict().items():
            if value != old_data[key]:
                logging.info(f'Updating "{key}" for {comment.startrek_id} ({comment.issue_key}) in DB. ' + \
                             f'Old: {old_data[key]}, New: {value}')
                new_value = value if key in BOOLEAN_KEYS else f'"{value}"'
                sql = f'UPDATE `{table}` ' + \
                      f'SET {key} = {new_value} ' + \
                      f'WHERE startrek_id = "{comment.startrek_id}"'

                self.query(sql, close=True)

    def delete_comment(self, comment: object, table='comments'):
        pass

    # INCOMING MESSAGES

    def add_incoming_msg(self, message: object, table='inbox'):
        """Add incoming message to database."""
        sql = f'INSERT INTO `{table}` ' + \
              f'(startrek_id, issue_key, author, created_at) ' + \
              f'VALUES ("{message.startrek_id}", "{message.issue_key}", ' + \
              f'"{message.author}", "{message.created_at}")'

        logger.info(f'Trying to add incoming message {message.startrek_id} to DB.')
        return self.query(sql, close=True)

    def get_incoming_msg(self, message: object, table='inbox'):
        """Returns record as dict, if it exists."""
        sql = f'SELECT * ' + \
              f'FROM `{table}` ' + \
              f'WHERE startrek_id = "{message.startrek_id}"'

        result = self.query(sql, close=True)
        if not result:
            logging.debug(f'Incoming message {message.startrek_id} not found in DB.')
            return False
        return make_perfect_dict(result)

    def update_incoming_msg(self, message: object, old_data: dict, table='inbox'):
        """Updating only different values for incoming message in database."""
        for key, value in message.to_dict().items():
            if value != old_data[key]:
                logging.info(f'Updating "{key}" for {message.startrek_id} ({message.issue_key}) in DB. ' + \
                             f'Old: {old_data[key]}, New: {value}')
                new_value = value if key in BOOLEAN_KEYS else f'"{value}"'
                sql = f'UPDATE `{table}` ' + \
                      f'SET {key} = {new_value} ' + \
                      f'WHERE startrek_id = "{message.startrek_id}"'

                self.query(sql, close=True)

    def delete_incoming_msg(self, message: object, table='inbox'):
        pass

    # FEEDBACK

    def add_feedback(self, feedback: object, table='feedback'):
        """Add feedback in to database."""
        sql = f'INSERT INTO `{table}` ' + \
              f'(issue_key, general, completely, rapid, description, created_at) ' + \
              f'VALUES ("{feedback.issue_key}", {feedback.general}, {feedback.completely}, {feedback.rapid}, ' + \
              f'"{feedback.description}", "{feedback.created_at}")'

        logger.info(f'Trying to add feedback for {feedback.issue_key} to DB (table: {table}).')
        return self.query(sql, close=True)

    def get_feedback(self, feedback: object, table='feedback'):
        """Returns record as dict, if it exists."""
        sql = f'SELECT * ' + \
              f'FROM `{table}` ' + \
              f'WHERE issue_key = "{feedback.issue_key}"'

        result = self.query(sql, close=True)
        if not result:
            logging.debug(f'Feedback for {feedback.issue_key} not found in DB.')
            return False
        return make_perfect_dict(result)

    def update_feedback(self, feedback: object, old_data: dict, table='feedback'):
        """Updating only different values for feedback in DB."""
        for key, value in feedback.to_dict().items():
            if value != old_data[key]:
                logging.info(f'Updating "{key}" for FB {feedback.issue_key} in DB. Old: {old_data[key]}, New: {value}')
                new_value = value if key in BOOLEAN_KEYS else f'"{value}"'
                sql = f'UPDATE `{table}` ' + \
                      f'SET {key} = {new_value} ' + \
                      f'WHERE issue_key = "{feedback.issue_key}"'

                self.query(sql, close=True)

    def delete_feedback(self, feedback: object, table='feedback'):
        pass


    # COMPONENTS

    def add_component(self, component: object, table='components'):
        """Add component to database."""
        sql = f'INSERT INTO `{table}` (name) ' + \
              f'VALUES ("{component.name}")'

        logger.info(f'Trying to add component {component.name} to DB.')
        return self.query(sql, close=True)

    def get_component(self, component: object, table='components'):
        """Returns record as dict, if it exists."""
        sql = f'SELECT * ' + \
              f'FROM `{table}` ' + \
              f'WHERE name = "{component.name}"'

        result = self.query(sql, close=True)
        if not result:
            logging.debug(f'Component {component.name} not found in DB.')
            return False
        return result

    def update_component(self, component: object, table='components'):
        pass

    def delete_component(self, component: object, table='components'):
        pass

    # TAGS

    def add_tag(self, tag: object, table='tags'):
        """Add tag to database."""
        sql = f'INSERT INTO `{table}` (name) ' + \
              f'VALUES ("{tag.name}")'

        logger.info(f'Trying to add tag {tag.name} to DB.')
        return self.query(sql, close=True)


    def get_tag(self, tag: object, table='tags'):
        """Returns record as dict, if it exists."""
        sql = f'SELECT * ' + \
              f'FROM `{table}` ' + \
              f'WHERE name = "{tag.name}"'

        result = self.query(sql, close=True)
        if not result:
            logging.debug(f'Tag {tag.name} not found in DB.')
            return False
        return result

    def update_tag(self, tag: object, table='tags'):
        pass

    def delete_tag(self, tag: object, table='tags'):
        pass

    # RELATIONSHIPS

    def set_component(self, component: object, table='issue_components'):
        """Set relationship between the issue and component in database."""
        sql = f'INSERT INTO `{table}` (name, startrek_key) ' + \
              f'VALUES ("{component.name}", "{component.relationship}")'

        logger.info(f'Trying to set component {component.name} for {component.relationship}.')
        return self.query(sql, close=True)

    def check_component(self, component: object, table='issue_components'):
        """Returns record as dict, if it exists."""
        sql = f'SELECT * FROM `{table}` ' + \
              f'WHERE name = "{component.name}" AND startrek_key = "{component.relationship}"'

        result = self.query(sql, close=True)
        if not result:
            logging.debug(f'Relationship between the issue {component.relationship} ' + \
                         f'and component {component.name} not found in DB.')
            return False
        return result

    def unset_component(self, component: object, issue: object, table='issue_components'):
        pass

    def set_tag(self, tag: object, table='issue_tags'):
        """Set relationship between the issue and tag."""
        sql = f'INSERT INTO `{table}` (name, startrek_key) ' + \
              f'VALUES ("{tag.name}", "{tag.relationship}")'

        logger.info(f'Trying to set tag {tag.name} for {tag.relationship}.')
        return self.query(sql, close=True)

    def check_tag(self, tag: object, table='issue_tags'):
        """Returns record as dict, if it exists."""
        sql = f'SELECT * FROM `{table}` ' + \
              f'WHERE name = "{tag.name}" AND startrek_key = "{tag.relationship}"'

        result = self.query(sql, close=True)
        if not result:
            logging.debug(f'Relationship between the issue {tag.relationship} ' + \
                         f'and tag {tag.name} not found in DB.')
            return False
        return result

    def unset_tag(self, tag: object, issue: object, table='issue_tags'):
        pass
