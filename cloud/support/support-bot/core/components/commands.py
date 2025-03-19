#!/usr/bin/env python
"""This module contains bot commands."""

import os
import logging
from datetime import datetime
from calendar import monthrange

from telegram.ext.dispatcher import run_async

from core.objects.base import Base
from core.objects.user import User
from core.workers import SupKeeper, MaintenanceNotifier
from core.components.common import userhelp, in_progress_issues, general_schedule
from core.components.keyboards import edit_keyboard, main_keyboard, close_keyboard, duty_keyboard

from utils.config import Config as config
from utils.decorators import log_msg
from utils.permissions import only_cloud_staff, only_existing_users, admin_required

from ext.schedule import schedule_generator
from ext.schedule import print_schedule
from ext.quality import quality_generator

logger = logging.getLogger(__name__)


@log_msg
@only_cloud_staff
def start(update: object, context: object, user: object = None):
    """Start command func."""
    if user is None:
        user = User.de_json(update.message.chat.to_dict())

    if user._is_exist:
        update.message.reply_text('Мы уже знакомы.')
        return

    user.create()
    update.message.reply_text(
        text=f'Привет, <b>{user.telegram_login}</b>! \nНе забудь проверить свои настройки.',
        parse_mode='HTML')
    update.message.reply_text(
        text=f'<b>Меню</b>',
        parse_mode='HTML',
        reply_markup=main_keyboard()
    )


@log_msg
@only_cloud_staff
@only_existing_users
def bye(update: object, context: object, user: object = None):
    """Bye command func."""
    if user is None:
        user = User.de_json(update.message.chat.to_dict())

    user.delete()
    update.message.reply_text('Твои данные удалены. Если захочешь вернуться, начни с команды /start')


@log_msg
@only_cloud_staff
@admin_required
def debug_message(update: object, context: object, user: object = None):
    msg = update.message.text.split()
    try:
        count = msg[1]
    except IndexError:
        count = 10

    logs = os.popen(f'tail -{count} {config.LOGPATH}').read().strip() or 'Log is empty'

    try:
        reply = f'<code>{logs}</code>'
    except Exception:
        reply = logs
    update.message.reply_text(
        text=reply,
        parse_mode='HTML'
    )


@log_msg
@only_cloud_staff
@only_existing_users
def menu(update: object, context: object, user: object = None):
    """Just return main menu."""
    update.message.reply_text(
        text=f'<b>Меню</b>',
        parse_mode='HTML',
        reply_markup=main_keyboard()
    )


@log_msg
@run_async
@only_cloud_staff
@only_existing_users
def settings(update: object, context: object, user: object = None):
    """Return user config and edit button."""
    metadata = '\n'.join(f'{k}: {v}' for k, v in user.config.to_dict().items())
    update.message.reply_text(
        text=f'<b>Твои настройки</b>\n\n<code>{metadata}</code>',
        parse_mode='HTML',
        reply_markup=edit_keyboard()
    )


@log_msg
@run_async
@only_cloud_staff
@only_existing_users
def cloudsupport_force_notify(update: object, context: object, user: object = None):
    """Return opened issues based on user subscriptions."""
    _worker = SupKeeper()
    first = update.message.reply_text(text='Проверяю открытые задачи на основе твоих подписок...')
    message = _worker.force_notify(update, context, user)

    context.bot.edit_message_text(
        chat_id=user.telegram_id,
        message_id=first.message_id,
        text=message,
        parse_mode='HTML'
    )

@log_msg
@run_async
@only_cloud_staff
@only_existing_users
def cloudops_force_notify(update: object, context: object, user: object = None):
    """Return actual maintenance tasks."""
    _worker = MaintenanceNotifier()
    first = update.message.reply_text(text='Проверяю актуальные рассылки...')
    message = _worker.force_notify(update, context, user)

    context.bot.edit_message_text(
        chat_id=user.telegram_id,
        message_id=first.message_id,
        text=message,
        parse_mode='HTML'
    )


@log_msg
@run_async
@only_cloud_staff
@only_existing_users
def in_progress(update: object, context: object, user: object = None):
    """Return `In Progress` issues."""
    first = update.message.reply_text(text='Проверяю твои задачи в работе...')
    message = in_progress_issues(user) or 'У тебя нет задач в работе в очереди CLOUDSUPPORT.'
    context.bot.edit_message_text(
        chat_id=user.telegram_id,
        message_id=first.message_id,
        text=message,
        parse_mode='HTML'
    )


@log_msg
@run_async
@only_cloud_staff
@only_existing_users
def schedule(update: object, context: object, user: object = None):
    """Return YC Support schedule."""
    first = update.message.reply_text(text='Сейчас уточню...')
    message = general_schedule()
    context.bot.edit_message_text(
        chat_id=user.telegram_id,
        message_id=first.message_id,
        text=message,
        parse_mode='HTML',
        disable_web_page_preview=True
    )


@log_msg
@only_cloud_staff
@only_existing_users
def help_message(update: object, context: object, user: object = None):
    """Return help message."""
    message = userhelp(user)
    update.message.reply_text(
        text=message,
        parse_mode='HTML',
        reply_markup=close_keyboard()
    )

@log_msg
@run_async
@only_cloud_staff
@only_existing_users
@admin_required
def duty(update: object, context: object, user: object = None):
    """Schedule generation"""
    help_msg=f'<b>schedule generator for YC-Support-Bot:</b>\n\n<code>/duty [year] [month] ' + \
        '(args must be int)\nexample: /duty 2020 8 ' + \
        '\n\nmonth format: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12\n' + \
        'year format: 2020, 2021, ...\n</code>'
    msg = update.message.text.split()

    try:
        year = msg[1]
        month = msg[2]
        if int(year) < 2020 or int(month) > 12 or int(month) < 1:
            month = datetime.today().strftime("%m")
            year = datetime.today().strftime("%Y")
    except (IndexError, ValueError):
            update.message.reply_text(    
                text=help_msg,
                parse_mode='HTML'
            )
            month = datetime.today().strftime("%m")
            year = datetime.today().strftime("%Y")
            
    metadata = '\n'.join(x for x in print_schedule(schedule_generator(year,month)))
    update.message.reply_text(
        text=f'<b>Расписание на {year}/{month}</b>\ndd/mm/yyyy, day primary, day backup, night primary, night backup\n\n<code>{metadata}</code>',
        parse_mode='HTML',
        reply_markup=duty_keyboard()
    )


@log_msg
@run_async
@only_cloud_staff
@only_existing_users
def testapi(update: object, context: object, user: object = None):
    """Return YC Support schedule."""
    first = update.message.reply_text(text='Сейчас уточню...')
    message = test_api_1()

    context.bot.edit_message_text(
        chat_id=user.telegram_id,
        message_id=first.message_id,
        text=message,
        parse_mode='HTML',
        disable_web_page_preview=True
    )


@log_msg
@run_async
@only_cloud_staff
@only_existing_users
def quality(update: object, context: object, user: object = None):
    """Return YC Support quality."""
    help_msg = 'quality generator for YC Support\n\nusage: /quality user1,user2,etc month day1-day2 [year]\n' + \
           '(args must be int)\nexample: /quality chronocross 11 7-21 ' + \
           '\n\nmonth format: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12\n' + \
           'days format: 7-21\n' + \
           'year format: 2021'
    month = datetime.today().strftime("%m")
    year = datetime.today().strftime("%Y")
    msg = update.message.text.split()
    message = ''
    
    try:
        members = msg[1].split(',')
    except (IndexError, ValueError):
        update.message.reply_text(    
            text=help_msg,
            parse_mode='HTML'
        )
        return

    try:
        month = msg[2]
        if int(month) < 1 or int(month) > 12:
            month = datetime.today().strftime("%m")
    except (IndexError, ValueError):
        update.message.reply_text(    
            text=help_msg,
            parse_mode='HTML'
        )
        return 
    
    try:
        days = [int(day) for day in msg[3].split('-')]
        if len(days) != 2:
            update.message.reply_text(    
                    text=help_msg,
                    parse_mode='HTML'
                )
            return 
    except (IndexError, ValueError, TypeError):
        update.message.reply_text(    
            text=help_msg,
            parse_mode='HTML'
        )
        return
    
    try:
        year = msg[4]
        if int(year) < 2018 or int(year) > 2030:
            year = datetime.today().strftime("%Y")
    except (IndexError, ValueError):
        year = datetime.today().strftime("%Y")

    first = update.message.reply_text(text='Собираю информацию...')
    for login in members:
        try:
            quality_generator(login,month,days,year)
        except:
            update.message.reply_text(    
                text=f'Не удалось собрать информацию по {login}. ',
                parse_mode='HTML'
            )

    context.bot.edit_message_text(
        chat_id=user.telegram_id,
        message_id=first.message_id,
        text='Готово',
        parse_mode='HTML',
        disable_web_page_preview=True
    )
    
    for login in members:
        try:
            context.bot.send_document(user.telegram_id, open(f'csv/{login}.csv', 'rb'))
            os.remove(f'csv/{login}.csv')
        except:
            update.message.reply_text(    
                text=f'Не удалось передать CSV по {login}. ',
                parse_mode='HTML'
            )
