# coding: utf-8

from __future__ import unicode_literals

from decimal import Decimal

from cia_stuff.utils import iterables


class SalaryHistory(iterables.DateRangeable):
    date_field = 'dateFrom'
    value_field = 'salarySum'
    value_wrapper = Decimal


class GradeHistory(iterables.DateRangeable):
    date_field = 'dateFrom'


class BonusHistory(iterables.DateRangeable):
    date_field = 'effectiveDate'
    value_field = 'summ'
    value_wrapper = Decimal


class VestingRange(iterables.DateRangeable):
    date_field = 'date'
    value_field = 'amount'
    value_wrapper = Decimal


def get_salary_event(salary_history, date):
    event = SalaryHistory.get_closest_before_date(salary_history, date)
    return event and {
        'value': event['salarySum'],
        'currency': event['currency'],
        'date': event[SalaryHistory.date_field]
    }


def get_grade_event(grade_history, date):
    event = GradeHistory.get_closest_before_date(grade_history, date)
    if not event:
        return
    profession, level, scale = event['gradeName'].split('.')
    try:
        # может быть строка «Без грейда»
        level = int(level)
    except Exception:
        level = None
    return {
        'profession': profession,
        'level': level,
        'scale': scale,
        'date': event[GradeHistory.date_field]
    }
