"""
    Various wrappers on clickhouse requests
"""

import random
import logging
import time

from core.settings import SETTINGS
import requests


# auxiliarily function
def ts_to_event_date(ts):
    """Convert ts to datetime (for clickhouse queries)"""
    import datetime
    import pytz

    return datetime.datetime.fromtimestamp(int(ts), tz=pytz.timezone("Europe/Moscow")).strftime('%Y-%m-%d')


class EBaseId(object):
    """We can perform operation on different clickhouse databases"""
    GRAPHS_INTERNAL = 'graphs_internal'  # clickhouse in runtime cloud (group ALL_GENCFG_CLICKHOUSE)
    GRAPHS_DBAAS = 'graphs_dbaas'  # clickhouse in DBaaS as replacement of GRAPHS_INTERNAL
    ALL = [GRAPHS_INTERNAL, GRAPHS_DBAAS]


def run_query(query, shuffle=True, base_id=EBaseId.GRAPHS_INTERNAL, attempts=3):
    """
        Run arbitrary query to clickhouse. Not optimized for inserting a lot of data or recieving (due to precalculating result list).
        Use http insterface, because clickhouse do not have python bindings.

        :type query: str
        :type shuffle: bool

        :param query: query as text string
        :param shuffle: use random backend
    """

    assert base_id in EBaseId.ALL, 'Unknown clickhouse base id <{}> (must be one of <{}>'.format(base_id, ','.join(EBaseId.ALL))

    if base_id == EBaseId.GRAPHS_INTERNAL:
        clickhouse_instances = SETTINGS.services.clickhouse.instances
        request_cgi = dict()
        request_headers = dict()
    elif base_id == EBaseId.GRAPHS_DBAAS:
        clickhouse_instances = ['{}:{}'.format(x, SETTINGS.services.dbaas.gencfg_graphs.http_port) for x in SETTINGS.services.dbaas.gencfg_graphs.hosts]
        request_cgi = {
            'database': SETTINGS.services.dbaas.gencfg_graphs.database,
        }
        request_headers = {
            'X-ClickHouse-User': SETTINGS.services.dbaas.gencfg_graphs.user,
            'X-ClickHouse-Key': SETTINGS.services.dbaas.gencfg_graphs.password,
        }
    else:
        raise Exception('Unknown clickhouse base id <{}>'.format(base_id))

    clickhouse_instances = random.sample(clickhouse_instances, len(clickhouse_instances)) if shuffle else clickhouse_instances

    for attempt in xrange(attempts):
        instance = clickhouse_instances[attempt % len(clickhouse_instances)]

        try:
            logging.debug("Trying to run query on host {}. first 100 characters of query = [{}]".format(instance, query[:100]))

            query_start_ts = time.time()
            r = requests.post("http://{}/".format(instance), data=query, params=request_cgi, headers=request_headers)
            query_end_ts = time.time()
            logging.debug("Took {} seconds to run query on host {}. first 100 characters of query = [{}]".format(query_end_ts - query_start_ts, instance, query[:100]))

            if r.status_code != 200:
                if attempt == attempts - 1:
                    r.raise_for_status()
                else:
                    continue

            query_result = [line.split('\t') for line in r.text.strip('\n').split('\n') if line.strip('\n')]
            query_result = map(lambda x: tuple(x), query_result)

            return query_result
        except Exception as e:
            logging.error('{}: {} ({}, {}) ({})'.format(type(e), e, instance, attempt, clickhouse_instances))
            if attempt == attempts - 1:
                raise

    assert False


def run_query_dict(query_tpl, fields, shuffle=True, base_id=EBaseId.GRAPHS_INTERNAL):
    """Run query and convert result tuple to dict"""
    assert len(fields) == len(set(fields)), 'Found non-uniq fieldsa among <{}>'.format(','.join(fields))

    query = query_tpl.format(fields=', '.join(fields))

    result = run_query(query, shuffle=shuffle, base_id=base_id)

    return [dict(zip(fields, x)) for x in result]
