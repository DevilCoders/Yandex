# coding=utf-8
import os
try:
    from urllib.parse import urljoin
except ImportError:
    from urlparse import urljoin


import requests

from retry import retry
from juggler_sdk import JugglerApi, JugglerError

from antiadblock.tasks.tools.logger import create_logger


JUGGLER_API_V2_BASE_URL = 'http://juggler-api.search.yandex.net/v2/'
JUGGLER_REQUEST_HEADERS = {'Accept': 'application/json', 'Content-Type': 'application/json'}

EVENTS_API = 'http://juggler-push.search.yandex.net/events'
EVENTS_SOURCE = 'robot-antiadb'

CHECKS_API = 'http://juggler-api.search.yandex.net:8998/api'

JUGGLER_MONEY_HOST = 'antiadb_money'

logger = create_logger('juggler_utils')


class JugglerAPIRequestException(Exception):
    pass


@retry(exceptions=requests.exceptions.ConnectTimeout, tries=3, delay=5)
def send_event(host, service, status, description=''):
    reply = requests.post(
        EVENTS_API,
        json={
            'source': EVENTS_SOURCE,
            'events': [{
                'host': host,
                'service': service,
                'status': status,
                'description': description,
            }]
        }
    )
    event_status = reply.json()["events"][0]
    if event_status["code"] != 200:
        raise JugglerAPIRequestException(event_status["error"])


@retry(exceptions=requests.exceptions.ConnectTimeout, tries=3, delay=5)
def create_aggregate_checks(checks, mark, juggler_token=os.getenv('JUGGLER_TOKEN'), downtime_changed=True):
    """
        Juggler API надо использовать, как менеджер контекста, т.к на выходе будет вызван cleanup(),
        который удалит агрегаты, которые не были обновлены (устарели)
    """
    if not isinstance(checks, (list, tuple)):
        raise Exception('checks should be iterable')
    with JugglerApi(CHECKS_API, mark=mark, oauth_token=juggler_token, downtime_changed=downtime_changed) as api:
        for check in checks:
            try:
                api.upsert_check(check)
            except JugglerError:
                logger.exception('Exception occuried while updating check {}:{}'.format(check.host, check.service))
                logger.error(check.to_dict())
                raise


def get_check_history(check, since, until, statuses=('CRIT',), raw=False):
    history = {}
    raw_history = []
    has_more = True
    page = 0
    while has_more:
        response = requests.post(urljoin(JUGGLER_API_V2_BASE_URL, 'history/get_check_history'), headers=JUGGLER_REQUEST_HEADERS,
                                 json={'host': check['host'], 'service': check['service'],
                                       'statuses': list(statuses), 'since': since, 'until': until, 'page': page})

        history.update({int(c['status_time']): c['status'] for c in response.json().get('states', [])})
        if raw:
            raw_history.extend(response.json().get('states', []))
        has_more = response.json().get('has_more', False)
        page += 1
    return raw_history if raw else history
