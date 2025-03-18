# -*- encoding: utf-8 -*-
from __future__ import unicode_literals
from datetime import datetime

from ..compat import string_types


def cast_to_date_str(date):
    """Конвертирует дату или дату-время в строку с датой,
    если передана не строка.

    :param date|datetime|str|unicode date:
    :rtype: str|unicode

    """
    result = date

    if not isinstance(date, string_types):
        result = date.strftime('%Y-%m-%d')

    return result


def cast_from_date_str(datestr):
    """Конвертирует строку в объект даты.

    :param str|unicode datestr:
    :rtype: date

    """
    return datetime.strptime(datestr, '%Y-%m-%d').date()
