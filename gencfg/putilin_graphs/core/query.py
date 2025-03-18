import time
import random
import logging
import itertools

import requests

import utils


INTERNAL_CLICKHOUSE = dict(
    hosts=('sas1-1716.search.yandex.net:17352', 'man1-8406.search.yandex.net:17352', 'vla1-4552.search.yandex.net:17352'),
)

DBAAS_CLICKHOUSE = dict(
    hosts=('man-wwtufkajbj4qp4vv.db.yandex.net:8123', 'vla-elgry6um232uwdqo.db.yandex.net:8123'),
    database='gencfg_graphs',
    user='kimkim',
    password='aevohngaC0fahchaeh3yohp7eic',
)

class EBase(object):
    """We can perform operation on different clickhouse databases"""
    INTERNAL = 'internal'  # clickhouse in runtime cloud (group ALL_GENCFG_CLICKHOUSE)
    DBAAS = 'dbaas'  # clickhouse in DBaaS as replacement of GRAPHS_INTERNAL
    ALL = [INTERNAL, DBAAS]


def get_base_for_table(table_name):
    """Some tables are moved from gencfg clickhouse to dbaas clickhouse"""
#    if table_name in ('instanceusage_infreq_v2',):
    if table_name in ():
        return EBase.DBAAS
    else:
        return EBase.INTERNAL


def _simple_run_query(query, host, priority, stream, base_type=EBase.INTERNAL, request_timeout_seconds=None):
    if request_timeout_seconds is None:
        request_timeout_seconds = 5 * 60

    logging.debug("Trying to run query on host %s. first 500 characters of query = [%s]", host, query[:500].replace('\n', ' '))
    query_start_ts = time.time()

    url = 'http://{}/?priority={}'.format(host, priority)
    if base_type == EBase.DBAAS:
        url = '{}&user={}&password={}&database={}'.format(url, DBAAS_CLICKHOUSE['user'], DBAAS_CLICKHOUSE['password'], DBAAS_CLICKHOUSE['database'])

    r = requests.post(
        url,
        data=query, stream=stream, timeout=request_timeout_seconds,
    )

    query_end_ts = time.time()

    logging.debug("Took %.2f seconds to run query on host %s.", query_end_ts - query_start_ts, host)
    # if not stream:
    #     logging.debug("First 500 characters of response: %s", r.text[:500].replace('\n', ' '))
    # else:
    #     logging.debug("Got streaming response")

    return r


def _run_query(
        query,
        base_type=EBase.INTERNAL,
        shuffle=True, priority=1, retries=3, stream=False, hosts=None,
        retries_timeout_seconds=None, request_timeout_seconds=None,
):
    if hosts is None:
        if base_type == EBase.INTERNAL:
            hosts = INTERNAL_CLICKHOUSE['hosts']
        elif base_type == EBase.DBAAS:
            hosts = DBAAS_CLICKHOUSE['hosts']
        else:
            raise Exception('Critical exception: got unknown base_type <{}>'.format(base_type))
    elif isinstance(hosts, str):
        hosts = [hosts]

    random.seed()
    hosts = random.sample(hosts, len(hosts)) if shuffle else hosts

    retries_made = 0
    for host in itertools.cycle(hosts):
        try:
            retries_made += 1
            r = _simple_run_query(query, host, priority, stream, base_type=base_type, request_timeout_seconds=request_timeout_seconds)

            r.raise_for_status()
            return r
        except requests.RequestException:
            if retries_made >= retries:
                raise

            time.sleep(retries_timeout_seconds or 0)


def run_query_simple(query, base_type=EBase.INTERNAL, priority=1, hosts=None):
    return _run_query(query, base_type=base_type, priority=priority, hosts=hosts).text


def run_query_stream(
        query,
        base_type=EBase.INTERNAL,
        priority=1, hosts=None,
        retries_timeout_seconds=None, request_timeout_seconds=None,
):
    return _run_query(
        query,
        base_type=base_type,
        priority=priority, stream=True, hosts=hosts,
        retries_timeout_seconds=retries_timeout_seconds,
        request_timeout_seconds=request_timeout_seconds,
    )


def run_query(
        query,
        base_type=EBase.INTERNAL,
        shuffle=True, priority=1, retries=3, hosts=None,
        retries_timeout_seconds=None, request_timeout_seconds=None,
        raise_failed=True,
):
    try:
        r = _run_query(
            query,
            shuffle=shuffle, priority=priority, retries=retries, base_type=base_type, hosts=hosts,
            retries_timeout_seconds=retries_timeout_seconds,
            request_timeout_seconds=request_timeout_seconds,
        )

        return [line.split('\t') for line in r.text.strip('\n').split('\n') if line.strip('\n')]
    except Exception as e:
        if raise_failed:
            raise
        else:
            return []


# TODO: replace value with a nice json by using FORMAT JSON
def dict_run_query(query, key_idx=0, **kwargs):
    data = run_query(query, **kwargs)

    res = {}
    for row in data:
        key = row[key_idx]
        value = row[:key_idx] + row[key_idx + 1:]
        res[key] = value if len(value) > 1 else value[0]

    return res


def run_query_format_json(query, base_type=EBase.INTERNAL, priority=1):
    return _run_query(query, base_type=base_type, priority=priority).json()


def run_single_value_query(query, base_type=EBase.INTERNAL):
    data = run_query(query, base_type=base_type)
    assert len(data) <= 1

    if len(data) == 0 or (len(data) == 1 and len(data[0]) == 0):
        return None

    return data[0][0]


def run_single_int_query(query, base_type=EBase.INTERNAL):
    str_res = run_single_value_query(query, base_type=base_type)
    return int(str_res) if str_res is not None else None


def get_roughly_last_event_date_from_table(table_name, days_to_watch=64):
    offset_days = 0
    while offset_days <= days_to_watch:
        query = "SELECT eventDate FROM {table_name} WHERE eventDate >= '{ed}' LIMIT 1".format(
            table_name=table_name,
            ed=utils.get_ch_formatted_date_from_timestamp(int(time.time()) - offset_days * utils.DAY)
        )
        event_date = run_single_value_query(query, base_type=get_base_for_table(table_name))
        if event_date is not None:
            return event_date
        offset_days = (offset_days * 2) or 1

    return None
