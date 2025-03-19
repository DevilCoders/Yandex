#!/usr/bin/env python3
"""This module contains SupportNotifier, IncidentNotifier and MaintenanceNotifier class."""

import time
import logging

from datetime import datetime
from telegram.ext.dispatcher import run_async

from utils.config import Config
from utils.helpers import delta_from_strtime, today_as_string, issue_fmt, de_timestamp

from services.startrek import StartrekClient
from services.resps import RespsClient

from core.error import RespsError
from core.objects.user import User
from core.database import init_db_client
from core.components.keyboards import reopen_keyboard
from core.components.common import in_progress_issues
from core.constants import (MINUTE, HOUR, WEEKENDS, BASE_CLOUDABUSE_FILTER,
                            BASE_CLOUDSUPPORT_FILTER, BASE_YCLOUD_FILTER, BASE_OPS_FILTER,
                            DUTY_DAY1_STAFF, DUTY_DAY2_STAFF, DUTY_NIGHT1_STAFF, DUTY_NIGHT2_STAFF)

logger = logging.getLogger(__name__)


class SupportNotifier:
    """A worker for sending notifications about new issues to subscribers."""

    def __init__(self):
        logger.info(f'Loading worker {type(self).__name__}...')
        self._db_session = init_db_client
        self.client = StartrekClient(token=Config.STARTREK_TOKEN)
        self.next_sent = {}
        self.actual_duty = {
            'employees': [],
            'next_check': float('-inf')
        }

        logger.debug(f'Sent storage: {self.next_sent}, Duty: {self.actual_duty}')

    def ycloud_issues(self, limit=7):
        """Return opened issues from YCLOUD queue."""
        issues = self.client.search_issues(BASE_YCLOUD_FILTER) or []
        total = len(issues)

        if total == 0:
            return ''

        tickets = '\n'.join(f'- {issue_fmt(t.key, title=t.summary)}' for t in issues[:limit])
        message = f'<b>[{total}] YCLOUD:</b>\n{tickets}\n\n'
        return message

    def cloudsupport_issues(self, limit=7, pay_filter=[]):
        """Return opened issues from CLOUDSUPPORT queue."""
        issues = self.client.search_issues(BASE_CLOUDSUPPORT_FILTER) or []
        total = len(issues)

        if total == 0:
            return ''

        # formatting output
        pretty_issues = []
        for ticket in issues:
            if pay_filter:
                if ticket.pay not in pay_filter:
                    continue

            result = ''
            time_left = '<code>--:--:--</code>'
            for timer in ticket.sla:
                sla = timer.get('failAt')
                if sla is not None:
                    time_left = delta_from_strtime(sla, datetime.now())
            result += f'<code>{time_left}</code> – '
            if pay_filter and ticket.companyName:
                company = ticket.companyName.replace('"', '') or None
                company = ticket.companyName.replace('<no billing account>', 'no billing account') or None
                result += f'{issue_fmt(ticket.key, title=ticket.summary, company=company, pay=ticket.pay)}'
            else:
                result += f'{issue_fmt(ticket.key, title=ticket.summary, pay=ticket.pay)}'
            pretty_issues.append(result) if result != '' else None

        if pay_filter and len(pretty_issues) == 0:
            return ''

        tickets = '\n'.join(pretty_issues[:limit])
        if pay_filter == ['premium']:
            message = f'<b>[{len(pretty_issues)}] PREMIUM SUPPORT:</b>\n{tickets}\n\n'
        elif pay_filter == ['business']:
            message = f'<b>[{len(pretty_issues)}] BUSINESS SUPPORT:</b>\n{tickets}\n\n'
        elif pay_filter == ['standard']:
            message = f'<b>[{len(pretty_issues)}] STANDARD SUPPORT:</b>\n{tickets}\n\n'
        else:
            message = f'<b>[{total}] CLOUDSUPPORT:</b>\n{tickets}\n\n'
        return message

    def cloudabuse_issues(self, limit=5, ):
        """Return opened issues from CLOUDABUSE queue."""
        issues = self.client.search_issues(BASE_CLOUDABUSE_FILTER) or []
        total = len(issues)

        if total == 0:
            return ''

        tickets = '\n'.join(f'- {issue_fmt(t.key, title=t.summary)}' for t in issues[:limit])
        message = f'<b>[{total}] CLOUDABUSE РОСКОМНАДЗОР:</b>\n{tickets}\n\n'
        return message

    def get_duty(self, current_time: int):
        """Pull current duty list from Resps."""
        check_time = self.actual_duty['next_check']
        current_day = datetime.now().day
        checkout_day = datetime.fromtimestamp(check_time).day if isinstance(check_time, datetime) else current_day
        order = 0 if 10 <= datetime.fromtimestamp(current_time).hour < 22 else 1

        # force check duty on 10:00 and 22:00  YCLOUD-3945
        if datetime.fromtimestamp(current_time).hour in (10, 22) and datetime.fromtimestamp(current_time).minute == 0 \
                and datetime.fromtimestamp(check_time).hour in (10, 22) \
                and datetime.fromtimestamp(current_time).minute < 5:
            check_time = 1

        if checkout_day != current_day or check_time < current_time:
            since_until = today_as_string(shift=0)
            try:
                duty_list = RespsClient().get_service_duty(
                    service='support',
                    since=since_until,
                    until=since_until
                )

            except RespsError as error:
                logging.error(error)
                oncall = []  # generate local in future
            else:
                oncall = [usr.resp.username for usr in duty_list if
                          usr.datestart <= current_time < usr.dateend and usr.resp.order == order]
            finally:
                # manual day duty list
                for i in oncall:
                    if i in DUTY_DAY1_STAFF:
                        oncall.extend(list(DUTY_DAY1_STAFF))
                        break
                    elif i in DUTY_DAY2_STAFF:
                        oncall.extend(list(DUTY_DAY2_STAFF))
                        break
                    elif i in DUTY_NIGHT1_STAFF:
                        oncall.extend(list(DUTY_NIGHT1_STAFF))
                        break
                    elif i in DUTY_NIGHT2_STAFF:
                        oncall.extend(list(DUTY_NIGHT2_STAFF))
                        break

                oncall = list(set(oncall))
                self.actual_duty['employees'] = oncall

            self.actual_duty['next_check'] = current_time + (5 * MINUTE)
            logger.info(f'Updating next checkout for actual duty. Oncall list: {oncall}')

        return self.actual_duty.get('employees', [])

    def pull_issues(self):
        """Get issues from Startrek."""
        self.cloudsupport = self.cloudsupport_issues()
        self.premium_cloudsupport = self.cloudsupport_issues(pay_filter=['premium'])
        self.business_cloudsupport = self.cloudsupport_issues(pay_filter=['business'])
        self.standard_cloudsupport = self.cloudsupport_issues(pay_filter=['standard'])
        #        self.new_premium_cloudsupport_important = self.cloudsupport_important(pay_filter=['premium'])
        #        self.new_business_cloudsupport_important = self.cloudsupport_important(pay_filter=['business'])
        self.cloudabuse = self.cloudabuse_issues()
        self.ycloud = self.ycloud_issues()

    def prepare_message(self, user: object):
        """Generates a message based on user subscriptions."""
        logger.info(f'Preparing message for {user.telegram_login}')
        message = ''

        #        message += self.new_premium_cloudsupport_important
        #        message += self.new_business_cloudsupport_important

        # if user.config.not_subscriber:
        #     logger.info(f'User {user.staff_login} not subscribed to notifications')
        #     return

        if user.config.cloudabuse:
            message += self.cloudabuse

        if user.config.premium_support:
            message += self.premium_cloudsupport

        if user.config.business_support:
            message += self.business_cloudsupport

        if user.config.standard_support:
            message += self.standard_cloudsupport

        if user.config.cloudsupport:
            message += self.cloudsupport

        if user.config.ycloud:
            message += self.ycloud

        if message != '':
            return message
        logger.info(f'There is no new information for the user {user.telegram_login}')
        return

    def send_message(self, user: object, context: object, current_time: int):
        message = self.prepare_message(user)

        logger.info(f'message for {user.staff_login}: {message}')

        if message is None:
            return

        context.bot.send_message(chat_id=user.telegram_id, text=message, parse_mode='HTML')
        logger.info(f'The notify message was sent to {user.telegram_login}')

        self.next_sent[user.telegram_id] = current_time + (user.config.notify_interval * MINUTE)
        logger.info(f'Set last notify sent for {user.telegram_login} to {datetime.fromtimestamp(current_time)}')

    def user_processing(self, user: object, context: object):
        current_time = int(time.time())
        current_hour = datetime.now().hour
        current_weekday = datetime.now().isoweekday()
        actual_duty = self.get_duty(current_time)

        # checkout dismissed users
        if not user.is_allowed:
            logger.warning(f'DISMISSED USER FOUND! Telegram ID: {user.telegram_id}, Staff: {user.staff_login}')
            return
        # checkout do not disturb
        elif user.config.do_not_disturb:
            logger.info(f'User {user.telegram_login} does not want to be disturbed')
            return
        # checkout interval
        elif self.next_sent.get(user.telegram_id, float('-inf')) > current_time:
            logger.info(f'The interval timeout is not over yet for {user.telegram_login}')
            return
        # checkout schedule are ignored
        elif user.config.ignore_work_time:
            logger.info(f'The user {user.telegram_login} wants to receive messages at any time')
            return self.send_message(user, context, current_time)
        # checkout the current duty
        elif user.config.is_duty and user.staff_login not in actual_duty:
            logger.info(f'User {user.telegram_login} is not actual duty')
            return
        # checkout weekends
        elif not user.config.is_duty and current_weekday in WEEKENDS:
            logger.info(f'User {user.telegram_login} has a day off today')
            return
        # checkout user worktime
        if user.config.work_time == '22-10':  # night employees
            if current_hour not in range(22, 24) and current_hour not in range(0, 10):
                logger.info(f'User {user.telegram_login} is night duty, but it is day')
                return
        elif current_hour not in range(user.config.start_workday, user.config.end_workday):
            logger.info(f'User {user.telegram_login} is not currently working')
            return

        return self.send_message(user, context, current_time)

    @run_async
    def run(self, context):
        """Start notify worker."""
        logger.info(f'Starting worker {type(self).__name__}...')
        self.pull_issues()
        users = User.de_list(self._db_session().get_all_users(with_config=True))

        for user in users:
            try:
                self.user_processing(user, context)
            except Exception as error:
                logger.warning(f'error in send message to {user.telegram_login}: {error}')

        logger.info(f'The worker {type(self).__name__} has finished working')

    def force_notify(self, update: object, context: object, user: object):
        """Sends the user a message about open issues."""
        self.pull_issues()
        useful_text = self.prepare_message(user)
        empty_message = 'По твоим подпискам нет открытых задач.'
        message = useful_text if useful_text else empty_message
        return message


class MaintenanceNotifier:
    """A worker for sending notifications about maintenance tasks."""

    def __init__(self):
        logger.info(f'Loading worker {type(self).__name__}...')
        self._db_session = init_db_client
        self.client = StartrekClient(token=Config.STARTREK_TOKEN)
        self.actual_duty = {
            'employees': [],
            'next_check': float('-inf')
        }

    def get_duty(self, current_time: int):
        """Pull current duty list from Resps."""
        check_time = self.actual_duty['next_check']
        current_day = datetime.now().day
        checkout_day = datetime.fromtimestamp(check_time).day if isinstance(check_time, datetime) else current_day

        if checkout_day != current_day or check_time < current_time:
            since_until = today_as_string(shift=0)
            try:
                duty_list = RespsClient().get_service_duty(
                    service='support',
                    since=since_until,
                    until=since_until
                )
            except RespsError as error:
                logging.error(error)
                oncall = []  # generate local in future
            else:
                oncall = [usr.resp.username for usr in duty_list if usr.datestart <= current_time < usr.dateend]
            finally:
                self.actual_duty['employees'] = oncall

            self.actual_duty['next_check'] = current_time + (5 * MINUTE)
            logger.info(f'Updating next checkout for actual duty. Oncall list: {oncall}')

        return self.actual_duty.get('employees', [])

    def get_tasks(self, limit=10):
        """Return opened issues from CLOUDOPS queue."""
        issues = self.client.search_issues(BASE_OPS_FILTER) or []
        total = len(issues)

        if total == 0:
            logging.info('Maintenance tasks not found')
            return

        tickets = '\n'.join(f'- {issue_fmt(t.key, title=t.summary)}' for t in issues[:limit])
        message = f'<b>[{total}] Актуальные задачи CLOUDOPS:</b>\n{tickets}\n\n'
        return message

    def force_notify(self, update: object, context: object, user: object):
        """Sends the user a message about open issues."""
        useful_text = self.get_tasks()
        empty_message = 'Актуальные задачи CLOUDOPS отсутствуют.'
        message = useful_text if useful_text else empty_message
        return message

    def run(self, context: object):
        """Starting maintenance notify worker."""
        logger.info(f'Starting worker {type(self).__name__}...')
        users = User.de_list(self._db_session().get_all_users(with_config=True))
        message = self.get_tasks()

        for user in users:
            if message is None:
                break
            if not user.config.cloudops:
                continue

            # fix for YCLOUD-4169
            current_time = int(time.time())
            current_hour = datetime.now().hour
            current_weekday = datetime.now().isoweekday()
            actual_duty = self.get_duty(current_time)

            # checkout dismissed users
            if not user.is_allowed:
                logger.warning(f'DISMISSED USER FOUND! Telegram ID: {user.telegram_id}, Staff: {user.staff_login}')
                continue
            # checkout do not disturb
            elif user.config.do_not_disturb:
                logger.info(f'User {user.telegram_login} does not want to be disturbed')
                continue
            # checkout interval
            # elif self.next_sent.get(user.telegram_id, float('-inf')) > current_time:
            #     logger.info(f'The interval timeout is not over yet for {user.telegram_login}')
            #     continue
            # checkout schedule are ignored - send
            # elif user.config.ignore_work_time:
            #     logger.info(f'The user {user.telegram_login} wants to receive messages at any time')
            #     return self.send_message(user, context, current_time)
            # checkout the current duty
            elif user.config.is_duty and user.staff_login not in actual_duty:
                logger.info(f'User {user.telegram_login} is not actual duty')
                continue
            # checkout weekends
            elif not user.config.is_duty and current_weekday in WEEKENDS:
                logger.info(f'User {user.telegram_login} has a day off today')
                continue
            # checkout user worktime
            if user.config.work_time == '22-10':  # night employees
                if current_hour not in range(22, 24) and current_hour not in range(0, 10):
                    logger.info(f'User {user.telegram_login} is night duty, but it is day')
                    continue
            elif current_hour not in range(user.config.start_workday, user.config.end_workday):
                logger.info(f'User {user.telegram_login} is not currently working')
                continue

            try:
                context.bot.send_message(
                    chat_id=user.telegram_id,
                    text=message,
                    parse_mode='HTML',
                    reply_markup=reopen_keyboard()
                )
                logging.info(f'Message with maintenance tasks sent to {user.telegram_login}')
            except Exception as error:
                logger.warning(f'error in send message to {user.telegram_login}: {error}')

        logger.info(f'The worker {type(self).__name__} has finished working')


class InProgressNotifier:
    """A worker for sending notifications about in progress issues."""

    def __init__(self):
        logger.info(f'Loading worker {type(self).__name__}...')
        self._db_session = init_db_client
        self.client = StartrekClient(token=Config.STARTREK_TOKEN)
        self.sent = {}

    def get_duty(self, current_time: int):
        since_until = today_as_string(shift=0)
        try:
            duty_list = RespsClient().get_service_duty(
                service='support',
                since=since_until,
                until=since_until
            )
        except RespsError as error:
            logging.error(error)
            oncall = []  # generate local in future
        else:
            oncall = [usr.resp.username for usr in duty_list if usr.datestart <= current_time < usr.dateend]
        finally:
            return oncall

    def prepare_message(self, user: object):
        """Generates a message for user."""
        logger.debug(f'Preparing inprogress message for {user.telegram_login}')
        message = f'Привет, <b>{user.telegram_login}</b>! ' + \
                  f'\nТвой рабочий день закончился, но у тебя остались задачи в работе.\n\n'

        in_progress = in_progress_issues(user)
        if not in_progress:
            logger.info(f'In progress issues for {user.telegram_login} not found')
            return

        message += in_progress
        return message

    def user_processing(self, user: object, context: object):
        """Processing and send message to user."""
        current_time = int(time.time())
        current_hour = datetime.now().hour
        current_weekday = datetime.now().isoweekday()
        oncall = self.get_duty(current_time)

        if current_hour == user.config.end_workday:
            if self.sent.get(user.telegram_login) != current_weekday or self.sent.get(user.telegram_login) is None:
                self.sent[user.telegram_login] = current_weekday
                message = self.prepare_message(user)
                if message is None:
                    return

                if oncall:
                    duty = '\n<b>Сегодня дежурят:</b> '
                    duty += ', '.join(oncall)
                    message += duty

                context.bot.send_message(
                    chat_id=user.telegram_id,
                    text=message,
                    parse_mode='HTML',
                    reply_markup=reopen_keyboard()
                )

                logging.info(f'Message with in progress issues sent to {user.telegram_login}')

    def run(self, context: object):
        """Starting in progress notify workeer."""
        logger.info(f'Starting worker {type(self).__name__}...')
        users = User.de_list(self._db_session().get_all_users(with_config=True))

        for user in users:
            try:
                self.user_processing(user, context)
            except Exception as error:
                logger.warning(f'error in send message to {user.telegram_login}: {error}')

        logger.info(f'The worker {type(self).__name__} has finished working')
