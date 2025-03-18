# coding: utf-8
from __future__ import unicode_literals

from datetime import date, datetime

import six

GAP_DATETIME_FORMAT = '%Y-%m-%dT%H:%M:%S'


def serialize_period(period):
    """
    Преобразовать период в isoformat.
    """
    if isinstance(period, date):
        return period.isoformat()
    return period


def date_from_gap_dt_string(datetime_str):
    """
    Извлечь объект дату из гэповоской datetime строки.
    """
    return datetime.strptime(datetime_str, GAP_DATETIME_FORMAT).date()


def smart_fetch_logins(person_or_list):
    """
    Получить список логинов.
    @param person_or_list может быть:
        * объектом с атрибутом login,
        * строкой-логином
        * последовательностью объектов, имеющих атрибут login
        * последовательностью логинов

    """
    if isinstance(person_or_list, six.string_types):
        login = person_or_list
        return [login]
    return person_or_list
