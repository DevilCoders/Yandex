#!/usr/bin/env python3
"""This module contains keyboards."""

from telegram import InlineKeyboardButton, InlineKeyboardMarkup
from core.constants import sql_constants
from core.database import init_db_client
import logging
logger = logging.getLogger(__name__)

def main_keyboard():
    keyboard = [
        [
            InlineKeyboardButton('Тикеты в работе', callback_data='opened_issues'),
            InlineKeyboardButton('Открытые тикеты', callback_data='in_progress_issues'),
            InlineKeyboardButton('Актуальные рассылки', callback_data='maintenance_issues')
        ],
        [
            InlineKeyboardButton('Настройки', callback_data='settings'),
            InlineKeyboardButton('Помощь', callback_data='help')
        ],
        [
            InlineKeyboardButton('Взять квест в очередь', callback_data='volunteer')
        ],
        [
            InlineKeyboardButton('Выйти из очереди', callback_data='deserter')
        ],
        [
            InlineKeyboardButton('Посмотреть свои очереди', callback_data='check_quest')
        ],
    ]
    return InlineKeyboardMarkup(keyboard)


def edit_keyboard():
    keyboard = [
        [
            InlineKeyboardButton('Изменить', callback_data='settings_edit'),
            InlineKeyboardButton('Закрыть', callback_data='close')
        ]
    ]
    return InlineKeyboardMarkup(keyboard)


def duty_keyboard():
    keyboard = [
        [
            InlineKeyboardButton('Заливаем', callback_data='duty_upload'),
            InlineKeyboardButton('Отмена', callback_data='close')
        ]
    ]
    return InlineKeyboardMarkup(keyboard)


def reopen_keyboard():
    keyboard = [
        [
            InlineKeyboardButton('Снять исполнителя и переоткрыть', callback_data='unassignee_and_reopen'),
            InlineKeyboardButton('Ничего не делать', callback_data='close')
        ]
    ]
    return InlineKeyboardMarkup(keyboard)


def close_keyboard():
    keyboard = [
        [
            InlineKeyboardButton('Закрыть', callback_data='single_close')
        ]
    ]
    return InlineKeyboardMarkup(keyboard)


def settings_keyboard():
    keyboard = [
        [
            InlineKeyboardButton('PAID SUPPORT', callback_data='paid_settings'),
            InlineKeyboardButton('CLOUDSUPPORT', callback_data='cloudsupport'),
            InlineKeyboardButton('CLOUDABUSE', callback_data='cloudabuse')
        ],
        [
            InlineKeyboardButton('YCLOUD', callback_data='ycloud'),
            InlineKeyboardButton('CLOUDOPS DAILY', callback_data='cloudops'),
            InlineKeyboardButton('Рабочее время', callback_data='work_time')
        ],
        [
            InlineKeyboardButton('Интервал уведомлений', callback_data='notify_interval'),
            InlineKeyboardButton('Уведомлять всегда (24/7)', callback_data='ignore_work_time'),
            InlineKeyboardButton('Не беспокоить', callback_data='do_not_disturb')
        ],
        [
            InlineKeyboardButton('Закрыть меню', callback_data='cancel')
        ]
    ]

    return InlineKeyboardMarkup(keyboard)


def bool_keyboard():
    keyboard = [
        [
            InlineKeyboardButton('Включить', callback_data='true'),
            InlineKeyboardButton('Выключить', callback_data='false')
        ],
        [
            InlineKeyboardButton('Закрыть меню', callback_data='cancel')
        ]
    ]

    return InlineKeyboardMarkup(keyboard)


def paid_support_keyboard():
    keyboard = [
        [
            InlineKeyboardButton('premium', callback_data='premium_support'),
            InlineKeyboardButton('business', callback_data='business_support'),
            InlineKeyboardButton('standard', callback_data='standard_support'),
        ],
        [
            InlineKeyboardButton('Закрыть меню', callback_data='cancel')
        ]
    ]

    return InlineKeyboardMarkup(keyboard)


def interval_keyboard():
    keyboard = [
        [
            InlineKeyboardButton('5 мин.', callback_data='5'),
            InlineKeyboardButton('10 мин.', callback_data='10'),
        ],
        [
            InlineKeyboardButton('15 мин.', callback_data='15'),
            InlineKeyboardButton('20 мин.', callback_data='20'),
        ],
        [
            InlineKeyboardButton('30 мин.', callback_data='30'),
            InlineKeyboardButton('1 час.', callback_data='60')
        ],
        [
            InlineKeyboardButton('Закрыть меню', callback_data='cancel')
        ]
    ]

    return InlineKeyboardMarkup(keyboard)


def worktime_keyboard():
    keyboard = [
        [
            InlineKeyboardButton('08-17', callback_data='08-17'),
            InlineKeyboardButton('09-18', callback_data='09-18'),
            InlineKeyboardButton('10-19', callback_data='10-19')
        ],
        [
            InlineKeyboardButton('11-20', callback_data='11-20'),
            InlineKeyboardButton('12-21', callback_data='12-21'),
            InlineKeyboardButton('13-22', callback_data='13-22')
        ],
        [
            InlineKeyboardButton('14-23', callback_data='14-23'),
            InlineKeyboardButton('10-22', callback_data='10-22'),
            InlineKeyboardButton('22-10', callback_data='22-10')
        ],
        [
            InlineKeyboardButton('Закрыть меню', callback_data='cancel')
        ]
    ]

    return InlineKeyboardMarkup(keyboard)


def task_handle_keyboard():
    keyboard = [
        [
            InlineKeyboardButton('Берём!', callback_data='accept_quest'),
            InlineKeyboardButton('Да ну его', callback_data='decline_quest'),
        ]
    ]

    return InlineKeyboardMarkup(keyboard)


def volunteer_keyboard():
    quests_query = sql_constants['ACTUAL_QUESTS']
    dbc = init_db_client
    quests = dbc().query(quests_query, fetchall=True)
    questboard = []
    for q in quests:
        questboard.append(
            [InlineKeyboardButton(
                f"{q.get('name')} - {int(q.get('crew_warning_limit')) - int(q.get('party'))} needed",
                callback_data=q.get('q_id')),])
    return InlineKeyboardMarkup(questboard)


def deserter_keyboard(sk_uid):
    quests_query = sql_constants['GET_USERS_QUESTS'] % sk_uid['u_id']
    dbc = init_db_client
    quests = dbc().query(quests_query, fetchall=True)
    questboard = []
    logger.info(f'QUESTBOARD============{questboard}\n====SK_UID============{sk_uid}\n====={quests_query}')
    for q in quests:
        questboard.append(
            [InlineKeyboardButton(
                f"{q.get('name')}",
                callback_data=f"{q.get('q_id')}_d"), ])
    return InlineKeyboardMarkup(questboard)

