import os
import math
import argparse
from urllib.parse import urljoin
from datetime import datetime, timedelta
from collections import defaultdict, Counter

import requests
from retry import retry

from startrek_client import Startrek
from statface_client import StatfaceClient, STATFACE_PRODUCTION

from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.common_configs import STARTREK_DATETIME_FMT
from antiadblock.tasks.tools.juggler import JUGGLER_API_V2_BASE_URL, JUGGLER_REQUEST_HEADERS, get_check_history
from antiadblock.tasks.tools.configs_api import RequestRetryableException, RequestFatalException

logger = create_logger('monitoring_stats')

STARTREK_TOKEN = os.getenv('STARTREK_TOKEN')
STATFACE_TOKEN = os.getenv('STAT_TOKEN')
DATE_FORMAT = '%Y-%m-%d'
ANTIADB_JUGGLER_NAMESPACE_NAME = 'antiadb'
MONITORING_STATS_REPORT = 'AntiAdblock/monitoring_stats'
UTC_HOURS_OFFSET = 3
MONTH_DAYS = 31


def ts_from_st_dt_field(field):
    dt = datetime.strptime(field.split('.')[0], STARTREK_DATETIME_FMT) + timedelta(hours=UTC_HOURS_OFFSET)
    return int(dt.strftime('%s'))


def get_value_by_ts(ts, dict_):
    return dict_.get(ts) or dict_.get(ts+60) or dict_.get(ts-60)


@retry(tries=3, delay=1, backoff=3, exceptions=(RequestRetryableException, requests.exceptions.ConnectionError), logger=logger)
def get_juggler_checks(limit=1000, offset=0):
    # https://juggler.yandex-team.ru/doc/#/checks//v2/checks/get_checks_config
    response = requests.post(urljoin(JUGGLER_API_V2_BASE_URL, 'checks/get_checks_config'),
                             headers=JUGGLER_REQUEST_HEADERS,
                             json={'filters': [{'namespace': ANTIADB_JUGGLER_NAMESPACE_NAME}],
                                   'limit': limit, 'offset': offset})

    if response.status_code in (502, 503, 504):
        raise RequestRetryableException("Unable to make juggler api request: response code is {}".format(response.status_code))

    if response.status_code != 200:
        raise RequestFatalException("{code} {text}".format(code=response.status_code, text=response.text))

    return response.json()


def get_all_juggler_checks():
    limit = 1000
    offset = 0
    total = math.inf
    items = []
    while offset < total:
        logger.debug(f'Get juggler checks: limit={limit}, offset={offset}, total={total}')
        data = get_juggler_checks(limit=limit, offset=offset)
        total = data.get('total', 0)
        chunk = data.get('items', [])
        offset += len(chunk)
        items += chunk

    logger.debug(f'Total count checks: {len(items)}')
    return items


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--date_range', nargs=2, metavar='%Y-%m-%d', default=None)
    args = parser.parse_args()
    if args.date_range:
        since = datetime.strptime(args.date_range[0], DATE_FORMAT)
        until = datetime.strptime(args.date_range[1], DATE_FORMAT)
    else:
        since = (datetime.now() - timedelta(days=MONTH_DAYS*2))
        until = datetime.now()
    # Получаем все агрегаты из Джаглера
    logger.debug(f'since: {since}, until: {until}')
    st_client = Startrek(useragent='python', token=STARTREK_TOKEN)
    antiadb_checks = get_all_juggler_checks()
    logger.debug(f'checks: {antiadb_checks}')
    # Достаем из ST тикеты из очереди ANTIADBALERTS за тот же период
    query = 'Queue: ANTIADBALERTS "Incident start": >= "{}" Resolution: fixed'.format(
        since.strftime(DATE_FORMAT))
    logger.debug(f'ST query: {query}')
    issues = st_client.issues.find(query=query)
    logger.debug(f'issues: {issues}')
    stat_data = []

    grouped_issues = defaultdict(dict)
    # Сгруппируем тикеты по алертам
    # Получим словарь вида {'host:service': {ts1: issue_type, ts2: issue_type}}
    for i in issues:
        grouped_issues[i.summary.strip()].update({ts_from_st_dt_field(i.incidentStart): i.tags[0]})
    for check in antiadb_checks:
        check_issues = grouped_issues.get('{}:{}'.format(check['host'], check['service']), {})
        check_dict = defaultdict(Counter)
        crit_history = list(get_check_history(check, since.strftime('%s'), until.strftime('%s')).keys())
        for ts, type in check_issues.items():
            # Добавим в историю фейковые срабатывания для false_negative алертов, так как в Джаглере их естественно нет
            if type == 'false_negative':
                crit_history.append(ts)
        for ts in crit_history:
            # Timestamp округляем до минут, так как в стартреке мы храним срабатывания с минутной точностью
            ts -= ts % 60
            # Берем тег от тикета, если тикета нет - срабатывание не ложное
            ts_type = get_value_by_ts(ts, check_issues) or 'true_positive'
            date = datetime.fromtimestamp(ts).strftime(DATE_FORMAT)
            check_dict[date][ts_type] += 1
        for date, count in check_dict.items():
            stat_data.append({
                'fielddate': date,
                'host': check['host'],
                'service': check['service'],
                'tp_count': count['true_positive'],
                'fp_count': count['false_positive'],
                'fn_count': count['false_negative']
            })
    logger.debug(f'stat_data: {stat_data}')
    StatfaceClient(host=STATFACE_PRODUCTION, oauth_token=STATFACE_TOKEN). \
        get_report(MONITORING_STATS_REPORT).upload_data(scale='d', data=stat_data)
