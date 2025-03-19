import logging, requests, json
from telegram.ext.dispatcher import run_async
from utils.helpers import delta_from_strtime, issue_fmt
from datetime import datetime
import time
from core.constants import (WEEKENDS, SK_ISSUES_ENDPOINT, sql_constants)
from utils.config import Config

from .notifier import SupportNotifier
from ..objects.sk_user import SK_User

logger = logging.getLogger(__name__)


class SupKeeper(SupportNotifier):
    """This is a worker to notify supkeeper users."""

    def get_issues(self, query, type):

        count_tickets_endpoint = SK_ISSUES_ENDPOINT
        st_token = Config.STARTREK_TOKEN
        headers = {
            'Authorization': f'OAuth {st_token}',
            'Content-Type': 'application/json'
        }
        req_type = type
        response = {}

        if req_type == "query":
            ticket_types = {
                'tickets_open': 'Status: Open',
                # may be useful later
                # 'tickets_in_progress': 'Status: inProgress',
                # 'tickets_sla_failed': 'Status: Open, inProgress Filter: 426514'
            }
            for k, v in ticket_types.items():
                body = {
                    req_type: f"{query} {v}"
                }
                # print(body)
                response[k] = requests.post(count_tickets_endpoint,
                                            headers=headers,
                                            json=body).json()
        elif req_type == "filter":

            ticket_types = {
                'tickets_open': '"status": "Open"',
                # may be useful later
                # 'tickets_in_progress': '"status": "inProgress"',
                # 'tickets_sla_failed': '"status": "Open,inProgress"'
            }
            print("FILTER TICKETS -================---------===========--------")
            for k, v in ticket_types.items():
                body = {
                    req_type: json.loads(f"{query[:-1]}, {v}}}")
                }
                print(body)
                response[k] = requests.post(count_tickets_endpoint,
                                            headers=headers,
                                            json=body).json()
        else:
            return response['tickets_open']

        return response['tickets_open']

    def s_keeper_issues(self, user: object, limit=10):
        sql = f'''
        {sql_constants['ST_ISSUES']} '%{user.staff_login}%'
        '''
        filters = self._db_session().query(sql, fetchall=True)

        message = ''
        for f in filters:
            issues = self.get_issues(f['ts_open'], f['query_type'])

            if type(issues) != list or len(issues) == 0:
                print(type(issues))
                message = f"{message} \n Очередь {f['name']} пуста \n"
                continue
            total = len(issues)
            pretty_issues = []
            for ticket in issues:
                result = ''
                time_left = '<code>--:--:--</code>'
                for timer in ticket['sla']:
                    sla = timer.get('failAt')
                    if sla is not None:
                        time_left = delta_from_strtime(sla, datetime.now())
                result += f'<code>{time_left}</code> – '
                if ticket.get('companyName'):
                    if ticket.get('companyName') == '<no billing account>':
                        company = "No BA"
                    elif ticket.get('companyName') == 'individual':
                        company = 'Физик'
                    else:
                        company = ticket['companyName'].replace('"', '')
                    result += f'{issue_fmt(ticket["key"], title=ticket["summary"], company=company, pay=ticket["pay"])}'
                else:
                    result += f'{issue_fmt(ticket["key"], title=ticket["summary"], pay=ticket["pay"])}'
                pretty_issues.append(result) if result != '' else None
            tickets = '\n'.join(pretty_issues[:limit])
            message = f'{message} <b>[{total}] {f["name"]}:</b> \n {tickets} \n'
        return message

    def prepare_message(self, user: object):
        """Generates a message based on user subscriptions."""
        logger.debug(f'Preparing message for {user.telegram_login}')
        message = ''

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

        message += self.s_keeper_issues(user)

        if message != '':
            return message
        logger.info(f'There is no new information for the user {user.telegram_login}')
        return

    def clear_absence(self):
        sql = """
        update s_keeper_support_unit set is_absent=0 where 1
        """
        try:
            self._db_session().query(sql)
        except BaseException as e:
            logger.warining(e.__dict__)
        return

    def user_processing(self, user: object, context: object, supkeeper=False):
        current_time = int(time.time())
        current_hour = datetime.now().hour
        current_weekday = datetime.now().isoweekday()
        actual_duty = self.get_duty(current_time)

        user.checkout_absence()

        # checkout dismissed users
        if not user.is_allowed:
            logger.warning(f'DISMISSED USER FOUND! Telegram ID: {user.telegram_id}, Staff: {user.staff_login}')
            return
        # checkout do not disturb
        elif user.config.do_not_disturb:
            logger.info(f'User {user.telegram_login} does not want to be disturbed')
            user.set_absent(1)
            return
        # checkout interval
        elif self.next_sent.get(user.telegram_id, float('-inf')) > current_time:
            logger.info(f'The interval timeout is not over yet for {user.telegram_login}')
            return
        # checkout schedule are ignored
        elif user.config.ignore_work_time:
            logger.info(f'The user {user.telegram_login} wants to receive messages at any time')
            user.set_absent(0)
            return self.send_message(user, context, current_time)
        # checkout the current duty
        elif user.config.is_duty and user.staff_login not in actual_duty:
            logger.info(f'User {user.telegram_login} is not actual duty')
            return
        # checkout weekends for 5/2
        elif not user.config.is_duty and current_weekday in WEEKENDS:
            logger.info(f'User {user.telegram_login} has a day off today')
            user.set_absent(1)
            return
        # checkout user worktime and abscence
        if user.config.work_time == '22-10':  # night employees
            if current_hour not in range(22, 24) and current_hour not in range(0, 10):
                logger.info(f'User {user.telegram_login} is night duty, but it is day')
                user.set_absent(1)
                return
        elif current_hour not in range(user.config.start_workday, user.config.end_workday):
            logger.info(f'User {user.telegram_login} is not currently working')
            user.set_absent(1)
            return

        return self.send_message(user, context, current_time)

    def user_keeper_sync(self, users):

        try:
            for user in users:
                if user['do_not_disturb']:
                    sql = f"""
                    {sql_constants['SET_DND']}'{user["staff_login"]}'
                    """
                    self._db_session().query(sql)
        except BaseException as error:
            logger.warning(error)
        return

    @run_async
    def run(self, context):
        """Start notify worker."""
        logger.info(f'Starting worker {type(self).__name__}...')
        self.pull_issues()
        self.clear_absence()
        users_with_config = self._db_session().get_all_users(with_config=True)
        self.user_keeper_sync(users_with_config)
        users = SK_User.de_list(users_with_config)
        for user in users:
            try:
                self.user_processing(user, context, supkeeper=True)
            except Exception as error:
                logger.warning(f'error in send message to {user.telegram_login}: {error.__dict__}')

        logger.info(f'The worker {type(self).__name__} has finished working')
