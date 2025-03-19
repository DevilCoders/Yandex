#!/usr/bin/env python3
"""This module contains TASK_NOTIFIER class."""

import logging
from telegram.ext.dispatcher import run_async

from core.components.keyboards import task_handle_keyboard
from core.constants import sql_constants, text_messages
from core.workers.supkeeper import SupKeeper
from core.objects.sk_user import SK_User
from core.database import init_db_client

logger = logging.getLogger(__name__)


class TaskNotifier(SupKeeper):

    def __init__(self):
        self._db_session = init_db_client
        logger.info(f'Loading worker {type(self).__name__}...')  # required

    def set_new_state(self):
        refresh_state = self._db_session().query(sql_constants['SET_NEW_TASKS_STATE'])
        return refresh_state

    @staticmethod
    def has_changes(user: object, changes):

        logins = [change['user'] for change in changes]
        print(logins)
        if user.staff_login in logins:
            return True
        logger.info('new changes pending')
        return False

    def get_changes(self):
        def prepare_change(qid, suid, ch):
            print(f'Input: {qid} {suid} {ch}')
            target_user_data = self._db_session().query(sql_constants['GET_TARGET_USER_ADD'] % suid) if ch == 'add' \
                else self._db_session().query(sql_constants['GET_TARGET_USER_RM'] % suid)

            queue_data = self._db_session().query(sql_constants['GET_QUEUE'] % qid)

            party_data = self._db_session().query(sql_constants['GET_PARTY_ADD'] % qid, fetchall=True) if ch == 'add' \
                else self._db_session().query(sql_constants['GET_PARTY_RM'] % qid, fetchall=True)

            print(f'DB response: {target_user_data} \n {queue_data} \n {party_data} \n')

            if party_data:
                colleagues = ' '.join([p.get('login') for p in party_data])
            else:
                colleagues = []
            pretty_change = {
                'change': ch,
                'user': target_user_data.get('login'),
                'user_id': target_user_data.get('u_id'),
                'queue_name':  queue_data.get('name') if queue_data else 'Deleted',
                'queue_id': queue_data.get('q_id') if queue_data else qid,
                'colleagues': colleagues,
                'supervisor': queue_data.get('last_supervisor')
            }
            return pretty_change

        sql = sql_constants['GET_ASSIGNMENT_CHANGES']

        changes = self._db_session().query(sql, fetchall=True)

        pretty_changes = []
        for change in changes:
            print(f'Unformatted change: {change}')
            if change['new_qfid'] is None and change['new_suid'] is None:
                try:
                    pretty_changes.append(
                        prepare_change(change['old_qfid'], change['old_suid'], 'rm')
                    )
                except BaseException:
                    logger.warning('Exception in add new removal')
            elif change['old_qfid'] is None and change['old_suid'] is None:
                try:
                    pretty_changes.append(
                        prepare_change(change['new_qfid'], change['new_suid'], 'add')
                    )
                except BaseException:
                    logger.warning('Exception in add new assignment')
            else:
                logger.warning('TaskNotifier fuckups. Check s_keeper_queue_state table')
        print(f'Formateed changes: {pretty_changes}')
        return pretty_changes

    def send_task(self, user: object, context: object, datas):
        try:
            context.bot.task_notifier_data[user.telegram_id] = datas
        except BaseException:
            context.bot.task_notifier_data = {user.telegram_id: datas}

        logger.info(f'datas - {context.bot.task_notifier_data}')
        for data in datas:
            if data.get('change') == 'add':
                context.bot.send_message(chat_id=user.telegram_id,
                                                  text=data.get('message'),
                                                  parse_mode='HTML',
                                                  reply_markup=task_handle_keyboard()
                                                  )
            elif data.get('change') == 'rm':
                context.bot.send_message(chat_id=user.telegram_id,
                                         text=data.get('message'),
                                         parse_mode='HTML')
                sql = None
                for q_id in data.get('queue_id').split(','):
                    sql = f"""
                           delete from s_keeper_queue_state 
                           where queue_filter_id like '{q_id}' and support_unit_id like '{data.get("user_id")}' 
                           """ if sql is None else f"""{sql} or queue_filter_id like '{q_id}' 
                           and support_unit_id like '{data.get("user_id")}' """
                self._db_session().query(sql)

            logger.info(f'The quest message {data.get("message")} was sent to {user.telegram_login}')
        return

    def prepare_message(self, user: object, changes=None):
        # sql = sql_constants['GET_SUPERVISORS']
        # supervisor = self._db_session().query(sql)
        logger.info('preparing messages')
        for change in changes:
            if change.get('change') == 'add':
                change['message'] = text_messages['PROPOSE_QUEST'] % (change.get("supervisor"), change["queue_name"])
            if change.get('change') == 'rm':
                logger.info(f'end quest for {user.staff_login}')
                change['message'] = text_messages['END_QUEST'] % (change["queue_name"])
        return changes

    @run_async
    def run(self, context: object):  # required, entry point for starting worker
        """Main worker func."""
        logger.info(f'Starting worker {type(self).__name__}...')  # required

        changes = self.get_changes()

        if changes is None:
            logger.info('No upcoming changes in TaskNotifier')
            return
        users = SK_User.de_list(self._db_session().get_all_users(with_config=True))
        logger.info(f'context user data: {context.user_data}')
        for user in users:
            logger.info(f'next user: {user}')
            if self.has_changes(user, changes):
                logger.info(f'user {user.staff_login} has upcoming changes')
                upcoming_changes = []
                for change in changes:
                    if change['user'] == user.staff_login:
                        upcoming_changes.append(change)
                logger.info(upcoming_changes)
                upcoming_changes = self.merge_changes(upcoming_changes)
                logger.info(upcoming_changes)
                self.send_task(user, context, self.prepare_message(user, upcoming_changes))

        logger.info(f'Worker {type(self).__name__} has finished working...')

    @staticmethod
    def merge_changes(changes):
        ch_add = {}
        ch_rm = {}
        for change in changes:
            if change.get('change') == 'add':
                ch_add = {
                    'change': 'add',
                    'user': change.get('user'),
                    'user_id': change.get('user_id'),
                    'queue_name': change.get('queue_name') if not ch_add else f'{ch_add.get("queue_name")}, '
                                                                              f'{change.get("queue_name")}',
                    'queue_id': change.get('queue_id') if not ch_add else f'{ch_add.get("queue_id")},'
                                                                          f'{change.get("queue_id")}',
                    'supervisor': change.get('supervisor') if not ch_add else f'{ch_add.get("supervisor")},'
                                                                          f'{change.get("supervisor")}',
                }

            if change.get('change') == 'rm':
                ch_rm = {
                    'change': 'rm',
                    'user': change.get('user'),
                    'user_id': change.get('user_id'),
                    'queue_name': change.get('queue_name') if not ch_rm else f'{ch_rm.get("queue_name")},'
                                                                             f' {change.get("queue_name")}',
                    'queue_id': change.get('queue_id') if not ch_rm else f'{ch_rm.get("queue_id")},'
                                                                         f'{change.get("queue_id")}',
                }
        return [ch_add, ch_rm]
