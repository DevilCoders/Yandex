# -*- coding: utf-8 -*-
import os
from datetime import datetime, timedelta

import pandas as pd

from startrek_client import Startrek

from antiadblock.tasks.tools.calendar import Calendar
import antiadblock.tasks.tools.common_configs as configs

daily_amount = 6 * 24  # количество 10-минуток в сутках
ACCEPTABLE_DATA_DELAY = 4  # hours

STARTREK_TOKEN = os.getenv('STARTREK_TOKEN')
STARTREK_QUERY_TEMPLATE = '''Components: {} Resolution: "won'tFix" Queue: ANTIADBSUP Resolved: >="{}" Tags: "device:{}"and Tags: "negative_trend" "Sort by": Resolved DESC'''
ANTIADB_SUPPORT_QUEUE = 'ANTIADBSUP'


def stat_dt_from_startrek_dt(st_dt, stat_dt_fmt=configs.STAT_FIELDDATE_I_FMT, st_dt_fmt=configs.STARTREK_DATETIME_FMT):
    return (datetime.strptime(st_dt.split('.')[0], st_dt_fmt) + configs.MSK_UTC_OFFSET).strftime(stat_dt_fmt)


def recheck_trend(service_id, device, stat_client, trend_border, delta=0.1):
    """
    ПРОВЕРКА НА ВАЛИДНОСТЬ ТРЕНДА ПО ЗАКРЫТЫМ ТИКЕТАМ
    :param service_id:
    :param device:
    :return: True|False
    """
    st_client = Startrek(useragent='python', token=STARTREK_TOKEN)
    component_id = next((component['id'] for component in st_client.queues[ANTIADB_SUPPORT_QUEUE].components if
                         component['name'] == service_id), None)

    since = (datetime.now() - timedelta(days=31)).strftime('%Y-%m-%d')
    issues = st_client.issues.find(query=STARTREK_QUERY_TEMPLATE.format(component_id, since, device))
    if not issues:
        # Если тикетов нет - тренд валидный
        return True
    last_issue = issues[0]
    baseline_start = stat_dt_from_startrek_dt(last_issue.createdAt)
    baseline_end = stat_dt_from_startrek_dt(last_issue.resolvedAt)
    ticket_data = stat_client.get_money_data(baseline_start, baseline_end)
    ticket_data = ticket_data.loc[service_id].sort_index()[:-1].query('device == "{}"'.format(device))
    ticket_data = ticket_data.unblock.copy()
    # Тикет мог быть открыт меньше одного дня
    window = min(daily_amount, ticket_data.shape[0])
    ticket_data_rolling_baseline = ticket_data.rolling(window).mean().min()
    return (ticket_data_rolling_baseline - trend_border) / ticket_data_rolling_baseline > delta


def check_no_data(partner_data, **kwargs):
    """
    ПРОВЕРКА НА ОТСУТСТВИЕ ДАННЫХ
    :param partner_data: timeseries
    :return:
    """
    now = datetime.now()
    no_data_hours = 0

    # на случай, если есть какие-то случайные данные
    if partner_data[now - timedelta(hours=ACCEPTABLE_DATA_DELAY + 2):].size < 5:
        no_data_hours = ACCEPTABLE_DATA_DELAY + 2  # за последние ACCEPTABLE_DATA_DELAY + 2 часа было лишь несколько случайных прострелов
    last_record_delay = (now - partner_data.index.max()).seconds // 3600
    no_data_hours = max(no_data_hours, last_record_delay)
    priority = 0
    if no_data_hours > ACCEPTABLE_DATA_DELAY:
        priority = - int(partner_data.aab_money.mean() * daily_amount)  # количество теряемых за сутки денег = (среднее количество денег за 10 минут) * 6 * 24 часа
    return {'check': 'no_data', 'status': 'CRIT' if priority != 0 else 'OK', 'info': no_data_hours, 'priority': priority}


def check_money_drop(partner_data, valuable=False, **kwargs):
    """
    ПРОВЕРКА НА НАЛИЧИЕ РЕЗКОГО ПАДЕНИЯ
    :param partner_data: timeseries
    :param valuable: enable strict check
    :return:
    """

    crit_threshold = 4  # количество точек в окне ПОДРЯД, при котором загорается CRIT
    force_crit_threshold = 10  # количество точек в окне, при котором загорается CRIT (порядок не важен)
    last_ts = partner_data.index.max()
    window = 2  # (часы) окно, процент разблока в котором мы сравниваем со средним историческим значением
    test_data = partner_data[last_ts - timedelta(hours=window):]
    history_days = 7  # количество дней, на основе которых определяем нормальный уровень процента разблока
    history_data_raw = partner_data[last_ts - timedelta(hours=history_days * 24 + window):last_ts - timedelta(hours=window)]
    # Оставим только выходные или будни в зависимости от момента оценки
    history_data = history_data_raw[history_data_raw['is_holiday'] == partner_data.loc[last_ts, 'is_holiday']]

    test_mean = test_data.unblock.mean()
    history_mean = history_data.unblock.mean()
    history_std = history_data.unblock.std()
    drop_threshold = history_mean - 3 * history_std
    is_drop = test_data.unblock[test_data.unblock < drop_threshold].count() >= force_crit_threshold
    if not is_drop:
        bad_count = 0
        for value in test_data.unblock:
            if value < drop_threshold:
                bad_count += 1
                if bad_count == crit_threshold:
                    is_drop = True
                    break
            else:
                bad_count = 0
    relative_drop = (history_mean - test_mean) / (history_mean + 1e-6)
    # количество теряемых за сутки денег
    priority = - int(relative_drop * partner_data.aab_money.mean() * daily_amount) if is_drop else 0
    return {'check': 'money_drop', 'status': 'CRIT' if is_drop else 'OK', 'info': int(relative_drop * 100), 'priority': priority}


def check_negative_trend(partner_data, **kwargs):
    """
    ПРОВЕРКА НА НАЛИЧИЕ НЕГАТИВНОГО ТРЕНДА
    :param partner_data: timeseries
    :return:
    """
    data = partner_data.unblock.copy()
    # делаем так, чтобы данные шли строго через 10 мин
    total_time_index = pd.date_range(partner_data.index.min(), partner_data.index.max(), freq='10min')
    holydays = Calendar().get_nonwork_days(days=40, weekends=False)
    holydays_mask = pd.Series(total_time_index).apply(lambda date: date.strftime("%Y-%m-%d") in holydays).values
    data = data.reindex(total_time_index)
    history_data = data[:-7 * daily_amount]
    # выбрасываем исторические данные с праздниками, потому что по праздникам процент разблокированых денег выше
    history_data[holydays_mask[:-7 * daily_amount]] = None
    # выбрасываем исторические данные с откровенными факапами
    history_data[history_data > 40] = None
    history_data[history_data <= history_data.mean() / 5] = None
    # заменяем пропущенные данные на среднее значение для того,
    # чтобы наличие резких падений, на которые мы уже проверяли,
    # не мешало построить тренд
    # (в исторических данных заменяем пропуски на среднее значение, а в данных за последнюю неделю - на ноль)
    history_data.fillna(value=history_data.mean(), inplace=True)
    data.fillna(value=0, inplace=True)
    # скользящее среднее с усреднением за неделю
    slow_rolling = data.rolling(window=7 * daily_amount).mean()[-30 * daily_amount:]
    fast_rolling = data.rolling(window=daily_amount).mean()[-30 * daily_amount:]
    # проверяем падение скользящего среднего за последние 7 дней (slow) и за день (fast)
    # относительно максимального значения недельного скользящего среднего
    slow_relative_drop = (slow_rolling.max() - slow_rolling[-1]) / (slow_rolling.max() + 1e-6)
    fast_relative_drop = (slow_rolling[-1] - fast_rolling[-1]) / (slow_rolling.max() + 1e-6)
    relative_drop = 0
    if fast_relative_drop >= 0.3:
        relative_drop = slow_relative_drop + fast_relative_drop
    elif (slow_relative_drop >= 0.14 and fast_relative_drop > -0.15) or slow_relative_drop + fast_relative_drop > 0.3:
        relative_drop = slow_relative_drop + fast_relative_drop
    elif fast_relative_drop <= -0.3:
        pass
    elif slow_relative_drop < 0.05 and fast_relative_drop < 0.15 or slow_relative_drop + fast_relative_drop < 0.1:
        pass
    else:
        return
    priority = - int(relative_drop * partner_data.aab_money.mean() * daily_amount) if relative_drop else 0  # количество теряемых за сутки денег
    if priority != 0 and not recheck_trend(kwargs['service_id'], kwargs['device'], kwargs['stat_client'], fast_rolling.min()):
        priority = 0
    status = 'CRIT' if priority != 0 else 'OK'
    return {'check': 'negative_trend', 'status': status, 'info': int(relative_drop * 100), 'priority': priority}
