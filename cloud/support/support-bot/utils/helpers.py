#!/usr/bin/env python3
"""This module contains helper functions."""

import re
import logging
import requests
from functools import wraps
from datetime import datetime, timedelta

from utils.decorators import retry
from core.constants import BOOLEAN_KEYS, BASE_SLA_FAILED_URL, BASE_SLA_WARNING_URL, BASE_REOPEN_URL, BASE_CHAT_URL

from requests.exceptions import ConnectionError, Timeout
from requests.packages.urllib3.exceptions import InsecureRequestWarning

logger = logging.getLogger(__name__)
requests.packages.urllib3.disable_warnings(InsecureRequestWarning)


class Emoji(enumerate):
    OK = u'✅'
    FIRE = u'🔥'
    LOCK = u'🔒'
    FAIL = u'❌'
    NONE = u'🚫'
    WARNING = u'❗️'
    ATTENTION = u'⚠️'


@retry((ConnectionError, Timeout))
def nda_link_generator(link):
    """Convert sensitive link to NDA."""
    url = 'https://nda.ya.ru/--'
    payload = {'url': link}
    r = requests.post(url, data=payload, verify=False)
    res = r.text
    return res


def make_perfect_dict(data: dict):
    """Convert tinyint to bool."""
    for k, v in data.items():
        if v == 'None':
            data[k] = None
        if k in BOOLEAN_KEYS:
            data[k] = bool(v)
    return data


def build_menu(buttons, n_cols, header_buttons=None, footer_buttons=None):
    menu = [buttons[i:i + n_cols] for i in range(0, len(buttons), n_cols)]
    if header_buttons:
        menu.insert(0, [header_buttons])
    if footer_buttons:
        menu.append([footer_buttons])
    return menu


def de_timestamp(timest: int, stringify=False):
    if not isinstance(timest, int):
        raise ValueError(f'Timestamp must be int, received: {type(timest)}')

    result = datetime.fromtimestamp(timest)
    if stringify:
        result = result.strftime('%Y-%m-%d %H:%M:%S')
    return result


def demojify(text: str):
    emoji_pattern = re.compile("["
            u"\U0001F600-\U0001F64F"  # emoticons
            u"\U0001F300-\U0001F5FF"  # symbols & pictographs
            u"\U0001F680-\U0001F6FF"  # transport & map symbols
            u"\U0001F1E0-\U0001F1FF"  # flags (iOS)
                            "]+", flags=re.UNICODE)
    return emoji_pattern.sub(r'', text)


def seconds_to_human_time(seconds, verbosity=2):
    """Convert seconds to human readable timedelta
    like a `2w 3d 1h 20m`

    """
    seconds = int(seconds)

    if seconds == 0:
        return 0.0

    negative = False
    if seconds < 0:
        negative = True
        seconds = abs(seconds)

    result = []
    intervals = (
        ('years', 31104000),
        ('months', 2592000),
        ('weeks', 604800),
        ('days', 86400),
        ('hrs', 3600),
        ('mins', 60),
        ('secs', 1),
    )
    for name, count in intervals:
        value = seconds // count
        if value:
            seconds -= value * count
            if value == 1:
                name = name.rstrip('s')
            result.append(f'{value} {name}')
    delta = ' '.join(result[:verbosity])
    return f'-{delta}' if negative else delta


def strfdelta(tdelta, fmt):
    d = {"days": tdelta.days}
    d["hours"], rem = divmod(tdelta.seconds, 3600)
    d["minutes"], d["seconds"] = divmod(rem, 60)

    return fmt.format(**d)


def make_datetime(dtime):
    """Convert string to datetime UTC +3."""
    moscow_tz = timedelta(hours=3)

    if isinstance(dtime, datetime):
        dtime = dtime.strftime('%Y-%m-%dT%H:%M:%S')

    result = datetime.strptime(dtime.split('.')[0], '%Y-%m-%dT%H:%M:%S') + moscow_tz
    return result


def get_raw_delta(d1, d2, out='minutes'):
    """
    d1: datetime - begin rime
    d2: datetime - end time
    out: str – must be `minutes` or `seconds`

    return: float - diff(end, begin)
    """
    delta = (d1 - d2).total_seconds()
    return delta // 60 if out == 'minutes' else delta


def delta_from_strtime(d1, d2, stringify=False):
    """
    d1: stringify datetime
    d2: stringify datetime
    stringify: bool
    """
    d1 = make_datetime(d1) if not isinstance(d1, datetime) else d1
    d2 = make_datetime(d2) if not isinstance(d2, datetime) else d2

    if stringify:
        delta = get_raw_delta(d1, d2, out='seconds')
        return seconds_to_human_time(delta)

    timer = (d1 - d2) if d2 < d1 else (d2 - d1)
    delta = strfdelta(timer, '{hours:02}:{minutes:02}:{seconds:02}')
    delta = f'{Emoji.WARNING}-{delta}' if d2 > d1 else delta
    return delta


def today_as_string(shift=None):
    today = datetime.now() + timedelta(hours=shift) if shift else datetime.now()

    today = today.strftime('%d-%m-%YT%H:%M')
    return today


def toweekday_as_string():
    """Return name  to day of week today."""
    toweekday = datetime.today().weekday()
    week = {0: 'monday',
            1: 'tuesday',
            2: 'wednesday',
            3: 'thursday',
            4: 'friday',
            5: 'saturday',
            6: 'sunday'
        }
    return week[toweekday]

def issue_fmt(issue_key: str, title=None, company=None, pay=None):
    """Pretty format for issue list."""
    if issue_key.lower().startswith('cloudsupport'):
        pay = pay or 'free'
        title = re.sub(r'[<>]', '', title) if title else issue_key
        title = f'[{company} - {pay}] {title[:20]}...' if company else f'{title[:25]}...'
    else:
        title = f'{title[:30]}...' if title else issue_key
    url = f'<a href="https://st.yandex-team.ru/{issue_key}">{title}</a>'
    return url

def roles_url_fmt(roles: str):
    """URL format for role list."""
    url = {
        'sla_failed': BASE_SLA_FAILED_URL,
        'sla_warning': BASE_SLA_WARNING_URL,
        'reopen': BASE_REOPEN_URL,
        'chat': BASE_CHAT_URL
    }
    role_name = {
        'sla_failed' : 'Протухшие',
        'sla_warning': 'Почти протухшие',
        'reopen': 'Переоткрытия',
        'chat': 'Чаты'
    }
    list_roles = []
    try:
        list_roles_tmp = roles.split(',')
#        print(list_roles_tmp)
    except (IndexError, ValueError, AttributeError):
        return roles

    for role in list_roles_tmp:
        if role in url:
            role_url = f'<a href = "{url[role]}">{role_name[role]}</a>'
#            print(f'role_url = ' + role_url)
            list_roles.append(role_url)
        else:
            list_roles.append(role)
    result = ','.join(list_roles)
    return result
