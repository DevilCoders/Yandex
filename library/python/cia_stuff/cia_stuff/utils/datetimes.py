# coding: utf-8

from __future__ import unicode_literals

from datetime import datetime, date

import arrow

now = arrow.now


def today():
    return now().date()


def today_shifted(**kwargs):
    return now().replace(**kwargs).date()


def parse_date(dt_str):
    if dt_str is None:
        return None
    return arrow.get(dt_str).date()


def convert_date_format(date_str):
    if not date_str:
        return
    dt = datetime.strptime(date_str, '%d.%m.%Y')
    return dt.date().isoformat()


def datetime_isoformat(dt):
    dt = arrow.get(dt)
    return dt.isoformat()[:len('YYYY-MM-DDTHH:MM:SS')]


def seconds_ago(dt):
    from datetime import datetime
    import pytz

    delta = (datetime.now(pytz.utc) - dt)
    return int(delta.total_seconds())


def shifted_dt(dt, **kwargs):
    dt = arrow.get(dt)
    return dt.replace(**kwargs)


def shifted_date(date, **kwargs):
    date = arrow.get(date)
    return date.replace(**kwargs).date()


def datetime_to_date(dt):
    return arrow.get(dt).date()


def days_between(dt_one, dt_two):
    dt_one = arrow.get(dt_one)
    dt_two = arrow.get(dt_two)
    return (dt_two - dt_one).days


def parse_date_isoformat(dt):
    return date(int(dt[:4]), int(dt[5:7]), int(dt[8:10]))
