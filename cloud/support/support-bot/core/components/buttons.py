#!/usr/bin/env python
"""This module contains buttons for TelegramBot."""

import logging, json
from telegram.ext.dispatcher import run_async

from core.database import init_db_client
from core.objects.user import User
from services.startrek import StartrekClient
from core.error import SelectionError, PermissionDenied, ValidateError
from core.components.common import userhelp, in_progress_issues
from core.workers.notifier import SupportNotifier, MaintenanceNotifier
from core.components.keyboards import (settings_keyboard, paid_support_keyboard,
                                       bool_keyboard, reopen_keyboard,
                                       worktime_keyboard, interval_keyboard,
                                       edit_keyboard, main_keyboard, close_keyboard, task_handle_keyboard,
                                       volunteer_keyboard, deserter_keyboard)
from ext.schedule import schedule_generator
from ext.schedule import push_schedule
from datetime import datetime
from core.constants import sql_constants, text_messages

logger = logging.getLogger(__name__)


class ActionMap:
    """This class provides an interface for calling actions based on user selection."""
    def __init__(self, update: object, context: object, user: object):
        self.update = update
        self.context = context
        self.user = user
        self.user_data = context.user_data
        self.query = update.callback_query
        self.choice = self.query.data

        if self.user is None or not self.user:
            raise ValidateError('NoneType user received')

        if not self.user.is_allowed:
            raise PermissionDenied(f'User {self.user.telegram_login} ({self.user.staff_login}) is not allowed.')

        self.CASES = {
            self.from_any_to_userconfig: ('settings',),
            self.from_config_to_settings: ('settings_edit',),
            self.from_upload: ('duty_upload',),
            self.from_settings_to_edit_boolean_values: (
                'ignore_work_time', 'do_not_disturb',
                'premium_support', 'business_support',
                'standard_support', 'cloudsupport',
                'cloudabuse', 'ycloud', 'cloudops'
            ),
            self.from_settings_to_edit_paid_settings: (
                'paid_settings'
            ),
            self.from_settings_to_edit_intervals: ('notify_interval',),
            self.from_settings_to_edit_worktime: ('work_time',),
            self.set_config_boolean_value: ('true', 'false'),
            self.set_config_timerange_value: (
                '08-17', '09-18', '10-19', '11-20', '12-21',
                '13-22', '14-23', '10-22', '22-10',
            ),
            self.set_config_int_value: (
                '5', '10', '15',
                '20', '30', '60',
            ),
            self.from_main_to_opened: ('opened_issues',),
            self.from_main_to_inprogress: ('in_progress_issues',),
            self.from_main_to_cloudops: ('maintenance_issues',),
            self.from_in_progress_to_reopen: ('unassignee_and_reopen',),
            self.from_main_to_help: ('help',),
            self.from_cancel_to_config: ('cancel',),
            self.from_close_to_main: ('close', 'single_close'),
            self.handle_quest: ('accept_quest', 'decline_quest'),
            self.volunteer: ('volunteer', 'deserter'),
            self.check_quest: ('check_quest',)
        }

    def check_quest(self):
        sk_uid = self.user._db_session().query(sql_constants['SK_TO_TG_ID'] % self.user.telegram_id)
        self.query.edit_message_text(
            text=text_messages["LIST_QUESTS"],
            parse_mode='HTML',
            reply_markup=deserter_keyboard(sk_uid)
        )

    def volunteer(self):
        sk_uid = self.user._db_session().query(sql_constants['SK_TO_TG_ID'] % self.user.telegram_id)

        if self.choice == 'deserter':
            self.query.edit_message_text(
                text=text_messages["WHICH_QUEST"],
                parse_mode='HTML',
                reply_markup=deserter_keyboard(sk_uid)
            )

        elif self.choice[-2:] == "_d":
            queue_name = self.user._db_session().query(sql_constants['GET_QUEUE'] % self.choice[:-2])
            sql_1 = sql_constants['SELFREMOVE1'] % (queue_name.get('q_id'), sk_uid.get('u_id'))
            sql_2 = sql_constants['SELFREMOVE2'] % (queue_name.get('q_id'), sk_uid.get('u_id'))

            logger.debug(f"""
            ==========QUEUE===REMOVE===== 
            {sql_1}
            {sql_2}
            ========
            """)
            self.user._db_session().query(sql_1)
            self.user._db_session().query(sql_2)
            self.send_message_to_supervisor(False, self.user.staff_login, queue_name.get('name'))
            self.query.edit_message_text(
                text=f'{text_messages["DROP_QUEST"]} {queue_name["name"]}',
                parse_mode='HTML',
                reply_markup=main_keyboard()
            )

        elif self.choice != 'volunteer':


            queue_name = self.user._db_session().query(sql_constants['GET_QUEUE'] % self.choice)

            logger.debug(f"Q_NAME_QUERY============={sql_constants['GET_QUEUE'] % self.choice}")

            assign_query1 = sql_constants['SELFASSIGN1'] % (self.choice, sk_uid.get('u_id'))
            assign_query2 = sql_constants['SELFASSIGN2'] % (self.choice, sk_uid.get('u_id'))
            sql_1 = sql_constants['SELFREMOVE1'] % (queue_name.get('q_id'), sk_uid.get('u_id'))
            sql_2 = sql_constants['SELFREMOVE2'] % (queue_name.get('q_id'), sk_uid.get('u_id'))
            logger.debug("removing duplicates if exist")
            self.user._db_session().query(sql_1)
            self.user._db_session().query(sql_2)
            logger.debug(f"ASSIGN_QUERY============={assign_query1}\n{assign_query2}")
            self.user._db_session().query(assign_query1)
            self.user._db_session().query(assign_query2)

            self.send_message_to_supervisor(True, self.user.staff_login, queue_name.get('name'))
            self.query.edit_message_text(
                text=f'{text_messages["TAKE_QUEST"]} {queue_name["name"]}',
                parse_mode='HTML',
                reply_markup=main_keyboard()
            )


        else:
            self.query.edit_message_text(
                text=text_messages["TAKE_QUEST_NOTE"],
                parse_mode='HTML',
                reply_markup=volunteer_keyboard()
            )


    def handle_quest(self):
        """
        self.context.bot.task_notifier_data = {
            'change': rm or add,
            'user': staff-login,
            'user_id': db login for s_keeper,
            'queue_name': human readable,
            'queue_id': for s_keeper db,
            'coallegues': string of other queue party
        }
        :return:
        """
        logger.debug(f'========NOTIFIER-DATA==\n{self.context.bot.task_notifier_data}\n===========')
        task_data = self.context.bot.task_notifier_data[self.user.telegram_id]

        data = task_data[0] # опасное дерьмо!
        sql_add, sql_cancel_add = None, None
        logger.debug(f"Data received by button processor: {task_data}")
        logger.debug(f"===================DATA SPLIT{data['queue_id'].split(',')}")
        for q_id in data["queue_id"].split(','):
            logger.debug(f"===============Q_ID: {q_id}")
            sql_add = f"""
            insert into s_keeper_queue_state (queue_filter_id, support_unit_id) 
            values ('{q_id}', '{data["user_id"]}') 
            """ if not sql_add else f"{sql_add},  (\'{q_id}\', \'{data['user_id']}\')"

            sql_cancel_add = f"""
            delete from s_keeper_queue_filter_assignees 
            where queue_filter_id like '{q_id}' and support_unit_id like '{data["user_id"]}' 
            """ if not sql_cancel_add else f"{sql_cancel_add} or queue_filter_id like \'{q_id}\' and " \
                                       f"support_unit_id like \'{data['user_id']}\'"
        if self.choice == 'accept_quest':
            self.query.edit_message_text(
                text=text_messages["QUEST_ACCEPTED"],
            )
            self.user._db_session().query(sql_add)

            self.send_message_to_supervisor(True, data.get('user'), data.get('queue_name'), data.get('supervisor'))
        elif self.choice == 'decline_quest':
            self.query.edit_message_text(
                text=text_messages["QUEST_DECLINED"]
            )
            logger.debug(sql_cancel_add)
            self.user._db_session().query(sql_cancel_add)
            self.send_message_to_supervisor(False, data.get('user'), data.get('queue_name'))

    def send_message_to_supervisor(self, value, user_id, queue_name, supervisor=None):
        sql = sql_constants['MESSAGE_TO_SV']
        sql_one = f"""
        {sql_constants['MESSAGE_TO_SV1']} '{supervisor}'
        """
        if value:
            message = text_messages["QUEST_ACCEPTED_NOTE"] % (user_id, queue_name)
        else:
            message = text_messages["QUEST_DECLINED_NOTE"] % (user_id, queue_name)


        supervisors = self.user._db_session().query(sql, fetchall=True)
        logger.debug(f"Уведомляем начальство: {supervisors}")
        for sv in supervisors:
            self.context.bot.send_message(chat_id=sv.get('telegram_id'),
                                     text=message,
                                     parse_mode='HTML')
        if supervisor:
            supervisor = self.user._db_session().query(sql_one)
            logger.debug(f'Concrete supervisor found: {supervisor}')
            logger.debug(f'Инфа про квест ущла в {supervisor}')
            self.context.bot.send_message(chat_id=supervisor.get('telegram_id'),
                                          text=message,
                                          parse_mode='HTML')

    def execute(self):
        for func, triggers in self.CASES.items():
            if self.choice in triggers:
                return func()

        try:
            return self.volunteer()
        except BaseException:
            error = f'Logic error in action map. Details: {self.user_data}. More: {self.query}'
            self.user_data.clear()
            raise SelectionError(error)

    def from_any_to_userconfig(self):
        self.user_data.clear()
        metadata = '\n'.join(f'{k}: {v}' for k, v in self.user.config.pull().to_dict().items())
        self.query.edit_message_text(
            text=f'<b>Твои настройки</b>\n\n<code>{metadata}</code>',
            parse_mode='HTML',
            reply_markup=edit_keyboard()
        )


    def from_in_progress_to_reopen(self):
        st_filter = f'Queue: CLOUDSUPPORT and Status: "In progress" and Assignee: {self.user.staff_login}'
        self.user_data.clear()
        self.query.edit_message_text(text='Снимаю исполнителя...')
        issues = StartrekClient().search_issues(issue_filter=st_filter)
        logger.debug(f'Trying reoopen issues for {self.user.telegram_login}, cause: dayoff button. Total: {len(issues)}')
        for issue in issues:
            issue.update(assignee={'unset': self.user.staff_login})
            issue.transitions['stop_progress'].execute()
        self.query.edit_message_text(
            text='Статус задач изменен на «Открыт»'
        )

    def from_main_to_opened(self):
        self.user_data.clear()
        self.query.edit_message_text(text='Проверяю твои задачи в работе...')
        message = in_progress_issues(self.user) or 'У тебя нет задач в работе в очереди CLOUDSUPPORT'
        self.query.edit_message_text(
            text=message,
            parse_mode='HTML'
        )

    def from_main_to_inprogress(self):
        self.user_data.clear()
        _worker = SupportNotifier()
        self.query.edit_message_text(text='Проверяю открытые задачи на основе твоих подписок...')
        message = _worker.force_notify(self.update, self.context, self.user)
        self.query.edit_message_text(
            text=message,
            parse_mode='HTML'
        )

    def from_main_to_cloudops(self):
        self.user_data.clear()
        _worker = MaintenanceNotifier()
        self.query.edit_message_text(text='Проверяю актуальные рассылки...')
        message = _worker.force_notify(self.update, self.context, self.user)
        self.query.edit_message_text(
            text=message,
            parse_mode='HTML'
        )

    def from_upload(self):
        msg = []
        try:
            logger.debug(f'starting upload from self.update: {self.update}')
            msg = self.update.callback_query.message.text.split()[2].split('/')
        except Exception as error:
            logger.err(f'error: {error}')

        self.user_data.clear()

        try:
            year = msg[0]
            month = msg[1]
            if int(year) < 2020 or int(month) > 12 or int(month) < 1:
                month = datetime.today().strftime("%m")
                year = datetime.today().strftime("%Y")
        except (IndexError, ValueError):
            month = datetime.today().strftime("%m")
            year = datetime.today().strftime("%Y")

        self.query.edit_message_text(text='Заливаем...')
        logger.debug(f'Before uploading log year {year}, month: {month}')
        try:
            message = '\n'.join(x for x in push_schedule(schedule_generator(year,month)))
        except Exception as error:
            message = 'Ошибка заливки'
            logger.error(f'Error uploading: {error}')
        self.query.edit_message_text(
            text=message,
            parse_mode='HTML',
            reply_markup=close_keyboard()
        )
        
    def from_main_to_help(self):
        self.user_data.clear()
        message = userhelp(self.user)
        self.query.edit_message_text(
            text=message,
            parse_mode='HTML',
            reply_markup=close_keyboard()
        )

    def from_cancel_to_config(self):
        self.user_data.clear()
        metadata = '\n'.join(f'{k}: {v}' for k, v in self.user.config.pull().to_dict().items())
        self.query.edit_message_text(
            text=f'<b>Твои настройки</b>\n\n<code>{metadata}</code>',
            parse_mode='HTML',
            reply_markup=edit_keyboard()
        )

    def from_close_to_main(self):
        self.user_data.clear()
        self.query.edit_message_text(
            text=f'<b>Просто оставлю это здесь</b>',
            parse_mode='HTML',
            reply_markup=main_keyboard()
        )

    def from_settings_to_edit_boolean_values(self):
        self.user_data['option'] = self.choice
        self.query.edit_message_text(
            text=f'<b>Изменение</b> <code>{self.choice}</code>',
            parse_mode='HTML',
            reply_markup=bool_keyboard()
        )

    def from_settings_to_edit_intervals(self):
        self.user_data['option'] = self.choice
        self.query.edit_message_text(
            text=f'<b>Изменение</b> <code>{self.choice}</code>',
            parse_mode='HTML',
            reply_markup=interval_keyboard()
        )

    def from_settings_to_edit_worktime(self):
        self.user_data['option'] = self.choice
        self.query.edit_message_text(
            text=f'<b>Изменение</b> <code>{self.choice}</code>',
            parse_mode='HTML',
            reply_markup=worktime_keyboard()
        )

    def from_config_to_settings(self):
        self.user_data['category'] = self.choice
        self.query.edit_message_text(
            text='<b>Изменение настроек</b>',
            parse_mode='HTML',
            reply_markup=settings_keyboard()
        )

    def from_settings_to_edit_paid_settings(self):
        self.query.edit_message_text(
            text='<b>Изменение параметров уведомлений</b>',
            parse_mode='HTML',
            reply_markup=paid_support_keyboard()
        )

    def set_config_timerange_value(self):
        self.user_data['param'] = self.choice
        self.query.edit_message_text(text='Изменяю параметры...')

        return self.update_and_return_userconfig()

    def set_config_int_value(self):
        self.user_data['param'] = int(self.choice) if self.choice.isdigit() else self.choice
        self.query.edit_message_text(text='Изменяю параметры...')

        return self.update_and_return_userconfig()

    def set_config_boolean_value(self):
        self.user_data['param'] = False if self.choice == 'false' else True
        self.query.edit_message_text(text='Изменяю параметры...')

        return self.update_and_return_userconfig()

    def update_and_return_userconfig(self):
        category = self.user_data.get('category', None)
        option = self.user_data.get('option', None)
        param = self.user_data.get('param', None)

        if len(self.user_data) < 3 or None in (category, option, param):
            self.context.bot.send_message(
                chat_id=self.user.telegram_id,
                text='Ошибка последовательности, вызови меню настроек повторно: /settings'
            )

        if category != 'settings_edit':
            logger.warning(f'Unknown query received: {self.user_data}, {self.query}')
            self.context.bot.send_message(
                chat_id=self.user.telegram_id,
                text='Что-то пошло не так'
            )
            return

        # updating userconfig in database
        self.user.config.update(option, param)
        # clearing user data and return new config
        self.context.user_data.clear()
        metadata = '\n'.join(f'{k}: {v}' for k, v in self.user.config.pull().to_dict().items())
        self.query.edit_message_text(
            text=f'<b>Твои настройки</b>\n\n<code>{metadata}</code>',
            parse_mode='HTML',
            reply_markup=edit_keyboard()
        )


@run_async
def main_button(update: object, context: object):
    """Callback function for telegram CallbackQuery."""
    query = update.callback_query
    telegram_user = query.message.chat.to_dict()
    logger.debug(f'=================={telegram_user} made {query}' )
    query.answer()
    user = User.de_json(telegram_user)
    logger.debug(f'===============BOT USER IS {user}')
    return ActionMap(update, context, user).execute()
