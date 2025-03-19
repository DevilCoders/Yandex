#!/usr/bin/env python
"""This module contains common functions for all components."""

import time
from datetime import datetime

from core.objects.user import User
from core.database import init_db_client as database
from services.startrek import StartrekClient
from services.resps import RespsClient
from services.staff import StaffClient
from utils.helpers import issue_fmt, delta_from_strtime, toweekday_as_string, today_as_string, roles_url_fmt


GENERAL_HELP = """
<b>Доступные команды</b>

<b>Регистрация</b>
/start – регистрация в боте
/bye – удалиться из бота

<b>Общее</b>
/menu – главное меню
/settings – персональные настройки

<b>Тикеты</b>
/me – твои тикеты в работе (cloudsupport)
/open – открытые задачи на основе подписок
/ops – актуальные рассылки (cloudops)

<b>Прочее</b>
/team – расписание команды
/duty – расписание дежурств
/logs {count} – посмотреть логи бота, где count (опционально) - кол-во строк

"""


def userhelp(user: object):
    message = GENERAL_HELP
    return message


def in_progress_issues(user: object):
    st_filter = f'Queue: CLOUDSUPPORT and Status: "In progress" and Assignee: {user.staff_login} "Sort By": SLA ASC'
    pretty_issues = []
    issues = StartrekClient().search_issues(issue_filter=st_filter) or []
    total = len(issues)

    for ticket in issues:
        result = ''

        time_left = f'<code>--:--:--</code>'
        for timer in ticket.sla:
            sla = timer.get('failAt')
            if sla is not None:
                time_left = delta_from_strtime(sla, datetime.now())

        result += f'<code>{time_left}</code> – {issue_fmt(ticket.key, title=ticket.summary, pay=ticket.pay)}'
        pretty_issues.append(result) if result != '' else None

    tickets = '\n'.join(pretty_issues)
    if not issues:
        return

    message = f'<b>[{total}] Задачи в работе CLOUDSUPPORT:</b>\n{tickets}\n'
    return message


def general_schedule():
    """Return schedule and actual duty."""
    current_time = int(time.time())
    resps = RespsClient()
    since_until = today_as_string(shift=2)
    message = '<b>Рабочее время команды YC Support</b>\n\n'
    oncall = []
    payload = []
    users = User.de_list(database().get_all_users(with_config=True,with_roles=True))

    try:
        duty_list = RespsClient().get_service_duty(
            service='support',
            since=since_until,
            until=since_until
        )
    except RespsError as error:
        logging.error(error)
        oncall = []
    else:
        oncall = [usr.resp.username for usr in duty_list if usr.datestart <= current_time < usr.dateend]

    if not users:
        return 'Список сотрудников пуст или что-то пошло не так...'

    for user in users:       
        if not user.is_support:
            continue
#        print(user.roles)
        user_roles = user.roles
        today_roles = user_roles[toweekday_as_string()]
        print(today_roles)
        if today_roles == '':
            today_roles = 'Ролей нет'
        elif today_roles == 'duty':
            today_roles = 'Дежурный'
        employee = f'<i>{user.config.work_time}</i> - ' + \
                   f'<a href="https://t.me/{user.telegram_login}">{user.staff_login}</a> - ' + \
                   f'<i>{roles_url_fmt(today_roles)}</i>'
#        print(user)
#        print(employee)
        payload.append(employee)

    message += '\n'.join(sorted(payload))

    if oncall:
        duty = '\n\n<b>Сегодня дежурят:</b> '
        duty += ', '.join(oncall) if oncall else ''
        message += duty

    return message


#def test_api_1():
#    """Return schedule and actual duty."""
#    print('НАЧАЛО ОБРАБОТКИ ФУНКЦИИ common.py/test_api_1______________________________')
#    current_time = int(time.time())
#    staff = StaffClient()
#    since_until = today_as_string(shift=2)
#    message = '<b>Отпуска команды</b>\n\n'
#    oncall = []
#    payload = []
#    users = User.de_list(database().get_all_users(with_config=True))
#
#    try:
#        duty_list = StaffClient().get_test_api_1(
#            service='support',
#            since=since_until,
#            until=since_until
#        )
#    except RespsError as error:
#        logging.error(error)
#        oncall = []
#    else:
#        oncall = [usr.resp.username for usr in duty_list if usr.datestart <= current_time < usr.dateend]
#
#    if not users:
#       return 'Список сотрудников пуст или что-то пошло не так...'
#
#    for user in users:
#       if not user.is_support:
#            continue
#        employee = f'<i>{user.config.work_time}</i> - ' + \
#                   f'<a href="https://t.me/{user.telegram_login}">{user.staff_login}</a>'
#        payload.append(employee)
#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!DICT
#    print(StaffClient().get_vacation())
    #!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!DICT-FCK-LOGIC
#    message += '\n'.join(str(StaffClient().get_vacation()))
#    print('ЗАПИСАЛИ ЧЕРЕЗ СТАФФКЛИЕНТ message______________________________________')
#    print('type: ', type(message))
#    print(message)
#
#    if oncall:
#        duty = '\n\n<b>Сегодня дежурят:</b> '
#        duty += ', '.join(oncall) if oncall else ''
#        message += duty
#
#    return message
