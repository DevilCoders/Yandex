#!/usr/bin/env python3
"""This module contains helper functions."""

import re
import time
import requests
import logging

from requests.exceptions import ConnectionError, Timeout

from functools import wraps
from datetime import datetime, timedelta
from requests.packages.urllib3.exceptions import InsecureRequestWarning
from metrics_collector.constants import BOOLEAN_KEYS

logger = logging.getLogger(__name__)
requests.packages.urllib3.disable_warnings(InsecureRequestWarning)


def make_datetime(dtime):
    """Convert string to datetime UTC +3."""
    moscow_tz = timedelta(hours=3)

    if isinstance(dtime, datetime):
        dtime = dtime.strftime('%Y-%m-%dT%H:%M:%S')

    result = datetime.strptime(dtime.split('.')[0], '%Y-%m-%dT%H:%M:%S') + moscow_tz
    return result


def get_delta_minutes(d1, d2, out='minutes'):
    """
    d1: datetime - Begin Time
    d2: datetime - EndT ime
    out: str – must be `minutes` or `seconds`

    return: float - diff(end, begin)
    """
    delta = (d1 - d2).total_seconds()
    return delta // 60 if out == 'minutes' else delta


def get_delta_time(d1, d2):
    """
    d1: float - Begin Time
    d2: float - EndT ime

    return: float - diff(end, begin)
    """
    timestamp_d1 = time.mktime(datetime.strptime(d1.split('.')[0], '%Y-%m-%dT%H:%M:%S').timetuple())
    timestamp_d2 = time.mktime(datetime.strptime(d2.split('.')[0], '%Y-%m-%dT%H:%M:%S').timetuple())
    return (timestamp_d2 - timestamp_d1) // 60


def make_simple_date(dtime, field=None):
    """
    resource: Startrek Resource - type with *At
    field: str or None

    return: str - (Unicode) date format YYYY-MM-DD HH:mm:ss
    """
    if field:
        dtime = dtime[field]
    timestamp = time.mktime(datetime.strptime(dtime.split('.')[0], '%Y-%m-%dT%H:%M:%S').timetuple())
    timestamp += 60 * 60 * 3  # shift offset to three hours
    return datetime.fromtimestamp(int(timestamp)).strftime('%Y-%m-%d %H:%M:%S')


def string_delta_time(seconds, verbosity=2):
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
        ('y', 31104000),
        ('mon', 2592000),
        ('w', 604800),
        ('d', 86400),
        ('h', 3600),
        ('m', 60),
        ('s', 1),
    )
    for name, count in intervals:
        value = seconds // count
        if value:
            seconds -= value * count
            result.append(f'{value}{name}')
    delta = ' '.join(result[:verbosity])
    return f'-{delta}' if negative else delta


def retry(exceptions, tries=5, delay=5, backoff=2):
    """Connection errors handling."""
    def retry_decorator(func):
        @wraps(func)
        def func_retry(*args, **kwargs):
            mtries, mdelay = tries, delay
            while mtries > 1:
                try:
                    return func(*args, **kwargs)
                except exceptions as e:
                    msg = f'Connection error. Retrying in {mdelay} seconds...'
                    print(msg)
                    time.sleep(mdelay)
                    mtries -= 1
                    mdelay *= backoff
            return func(*args, **kwargs)
        return func_retry
    return retry_decorator


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


def demojify(text: str):
    emoji_pattern = re.compile("["
            u"\U0001F600-\U0001F64F"  # emoticons
            u"\U0001F300-\U0001F5FF"  # symbols & pictographs
            u"\U0001F680-\U0001F6FF"  # transport & map symbols
            u"\U0001F1E0-\U0001F1FF"  # flags (iOS)
                            "]+", flags=re.UNICODE)
    return emoji_pattern.sub(r'', text)
