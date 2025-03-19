#!/usr/bin/env python3
"""This module contains SecurityWatchdog class."""

import logging
from telegram.ext.dispatcher import run_async

from core.database import init_db_client
from core.objects.user import User
from core.constants import DEVELOPERS

logger = logging.getLogger(__name__)


class SecurityWatchdog:
    """No one will leave alive.

    Checks whether data is up-to-date for all local users.
    If the user dismissed or rotated, it deletes them from the database, thereby closing access to the bot.
    """

    def __init__(self):
        logger.info(f'Loading worker {type(self).__name__}...')
        self.dismissed_users = []
        self._db_session = init_db_client

    def get_local_users(self):
        """Return users from database."""
        users = User.de_list(
            self._db_session().get_all_users(with_config=True),
            force_checkout=True
        )

        if not users:
            logging.warning('Users is empty')
            return []
        return users

    def report_to_developers(self, context: object):
        """Prepare and send report to developers."""
        total = len(self.dismissed_users)
        report = '\n'.join(self.dismissed_users)
        message = f'<b>Отчет по уволенным сотрудникам.</b> \nУдалено: {total}. \n\nСписок: \n<code>{report}</code>'

        if not self.dismissed_users:
            logging.info('Dismissed users not found in DB. So we can sleep soundly')
            return

        logger.info(f'Total dismissed users deleted: {total}')
        for dev in DEVELOPERS:
            context.bot.send_message(
                chat_id=dev,
                text=message,
                parse_mode='HTML'
            )

    @run_async
    def run(self, context: object):
        """Deleting dismissed users from database."""
        logger.info(f'Starting worker {type(self).__name__}...')

        for user in self.get_local_users():
            if not user.is_allowed:
                logging.warning(f'Dismissed user found: {user.telegram_id}, {user.telegram_login}')
                self.dismissed_users.append(f'{user.telegram_id}:{user.telegram_login}')
                user.delete()

        self.report_to_developers(context)
        logger.info(f'The worker {type(self).__name__} has finished working')
