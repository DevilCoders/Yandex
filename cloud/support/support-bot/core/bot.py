#!/usr/bin/env python3
"""This module contains TelegramBot class."""

import sys
import logging
import traceback

from telegram.ext import Updater
from telegram.utils.helpers import mention_html

from core.objects.base import Base
from core.constants import KNOWN_ERRORS, DEVELOPERS

logger = logging.getLogger(__name__)


class TelegramBot(Base):
    """This object represents a Telegram Bot.

    Arguments:
      :token: str
      :handlers: list
      :commands: list
      :repeating_workers: dict
      :daily_workers: dict

    """

    def __init__(self,
                 token: str = None,
                 handlers: list = None,
                 commands: list = None,
                 repeating_workers: dict = None,
                 daily_workers: dict = None,
                 **kwargs):

        self.token = token
        self.handlers = handlers or []
        self.commands = commands or []
        self.repeating_workers = repeating_workers or {}
        self.daily_workers = daily_workers or {}
        self._other = kwargs

    def error(self, update: object, context: object):
        """Redirect errors to developers."""
        try:
            if context.error in KNOWN_ERRORS:
                for developer in DEVELOPERS:
                    msg = f'<b>WARNING!</b> \nKNOWN ERROR RECEIVED FROM USER: \n<code>{update.effective_user}</code>'
                    context.bot.send_message(developer, msg, parse_mode='HTML')
                    context.bot.send_message(developer,context.error, parse_mode='HTML')
        except Exception as error:
            logger.warning(f'Unhandled error received: {error}')

        trace = ''.join(traceback.format_tb(sys.exc_info()[2]))
        payload = ''

        if update.effective_user:
            payload += f' with user {mention_html(update.effective_user.id, update.effective_user.username)}'
        if update.effective_chat:
            payload += f' in chat <i>{update.effective_chat.title}</i>'
            if update.effective_chat.username:
                payload += f' (@{update.effective_chat.username})'
        if update.poll:
            payload += f' with polling ID: {update.poll.id}'

        text = f'An error <code>{context.error}</code> occurred {payload}.\nDetails: \n<code>{trace}</code>'
        for developer in DEVELOPERS:
            try:
                context.bot.send_message(developer, text, parse_mode='HTML')
            except Exception:
                context.bot.send_message(developer, text)

    def stop_and_restart(self):
        """Soft restart bot."""
        pass

    def run(self, proxy: dict = None):
        """Start bot polling and workers."""
        logger.info('Starting bot...')
        updater = Updater(
            token=self.token,
            request_kwargs=proxy or {},
            use_context=True
        )
        dispatcher = updater.dispatcher
        dispatcher.add_error_handler(self.error)

        for handler in self.handlers:
            dispatcher.add_handler(handler)

        for command in self.commands:
            dispatcher.add_handler(command)

        for worker, params in self.repeating_workers.items():
            interval, first = params
            worker = worker().run
            updater.job_queue.run_repeating(worker, interval=interval, first=first)

        for worker, _time in self.daily_workers.items():
            worker = worker().run
            updater.job_queue.run_daily(worker, _time)

        updater.start_polling()
        logger.info('All modules loaded. Working...')
        # updater.idle()
