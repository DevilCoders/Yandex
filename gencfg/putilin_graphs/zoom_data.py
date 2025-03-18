import sys
import time
import random
import logging
import datetime
import itertools

import pytz
import requests


CLICKHOUSE_HOSTS = (
    "vla1-4552.search.yandex.net:17352",
    "sas1-1716.search.yandex.net:17352",
    # "man1-8406.search.yandex.net:17352",
)


def get_ch_formatted_date_from_timestamp(ts):
    return datetime.datetime.fromtimestamp(ts, tz=pytz.timezone("Europe/Moscow")).strftime('%Y-%m-%d')


def _simple_run_query(query, host, priority, stream, request_timeout_seconds=None):
    if request_timeout_seconds is None:
        request_timeout_seconds = 12 * 60 * 60

    logging.debug("Trying to run query on host %s. first 500 characters of query = [%s]", host, query[:500])
    query_start_ts = time.time()
    r = requests.post(
        "http://{}/?priority={}".format(host, priority),
        data=query, stream=stream, timeout=request_timeout_seconds,
    )
    query_end_ts = time.time()

    logging.debug(
        "Took %i seconds to run query on host %s. first 500 characters of query = [%s]",
        query_end_ts - query_start_ts, host, query[:500]
    )
    if not stream:
        logging.debug("First 500 characters of response: %s", r.text[:500])
    else:
        logging.debug("Got streaming response")

    return r


def _run_query(
        query,
        shuffle=True, priority=1, retries=3, stream=False,
        hosts=None, retries_timeout_seconds=None, request_timeout_seconds=None,
):
    if hosts is None:
        hosts = CLICKHOUSE_HOSTS
    elif isinstance(hosts, str):
        hosts = [hosts]

    hosts = random.sample(hosts, len(hosts)) if shuffle else hosts
    retries_made = 0
    for host in itertools.cycle(hosts):
        try:
            retries_made += 1
            r = _simple_run_query(query, host, priority, stream, request_timeout_seconds=request_timeout_seconds)
            r.raise_for_status()
            return r
        except requests.RequestException:
            if retries_made >= retries:
                raise

            time.sleep(retries_timeout_seconds or 0)


def run_query_simple(query, priority=1, hosts=None):
    return _run_query(query, priority=priority, hosts=hosts).text


def run_query_stream(
        query,
        priority=1, hosts=None,
        retries_timeout_seconds=None, request_timeout_seconds=None,
):
    return _run_query(
        query,
        priority=priority, stream=True, hosts=hosts,
        retries_timeout_seconds=retries_timeout_seconds,
        request_timeout_seconds=request_timeout_seconds,
    )


def run_query(
        query,
        shuffle=True, priority=1, retries=3,
        hosts=None, retries_timeout_seconds=None, request_timeout_seconds=None,
):
    r = _run_query(
        query,
        shuffle, priority, retries, hosts=hosts,
        retries_timeout_seconds=retries_timeout_seconds,
        request_timeout_seconds=request_timeout_seconds,
    )

    return [line.split('\t') for line in r.text.strip('\n').split('\n') if line.strip('\n')]


# TODO: replace value with a nice json by using FORMAT JSON
def dict_run_query(query, key_idx=0, **kwargs):
    data = run_query(query, **kwargs)

    res = {}
    for row in data:
        key = row[key_idx]
        value = row[:key_idx] + row[key_idx + 1:]
        res[key] = value if len(value) > 1 else value[0]

    return res


def run_query_format_json(query, priority=1):
    return _run_query(query, priority=priority).json()


def run_single_int_query(query):
    str_res = _run_single_value_query(query)
    return int(str_res) if str_res is not None else None


def _run_single_value_query(query):
    data = run_query(query)
    assert len(data) <= 1

    if len(data) == 0 or (len(data) == 1 and len(data[0]) == 0):
        return None

    return data[0][0]
