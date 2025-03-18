# -*- coding: utf-8 -*-

from datetime import datetime, time, timedelta
from django.utils import timezone

from core.forms import StatisticsForm
from core.settings import DEFAULT_STATS_PERIOD


def stat_dates(form):
    start_date = form.cleaned_data['start_date']
    end_date = form.cleaned_data['end_date']
    return date_range(start_date, end_date)


def default_stat_dates():
    cur_date = timezone.localtime(timezone.now()).date()
    start_date = cur_date - DEFAULT_STATS_PERIOD
    end_date = cur_date
    form = StatisticsForm(initial={
        'start_date': start_date, 'end_date': end_date
    })
    return date_range(start_date, end_date) + (form,)


def ang_period_start():
    cur_date = timezone.localtime(timezone.now()).date()
    if cur_date.day >= 15:
        start_date = cur_date.replace(day=15)
    else:
        prev_month_end = cur_date - timedelta(cur_date.day)
        start_date = prev_month_end.replace(day=15)
    return make_datetime(start_date)


def date_range(start_date, end_date):
    (start_date, end_date) = map(make_datetime, (start_date, end_date))
    end_date += timedelta(1)
    return (start_date, end_date)


def make_datetime(date):
    tz = timezone.get_current_timezone()
    return tz.localize(datetime.combine(date, time()))
