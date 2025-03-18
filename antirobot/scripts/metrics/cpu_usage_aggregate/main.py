import time
import requests
import logging
from datetime import datetime
from collections import defaultdict

import numpy as np
import click
import solomon
import retry
from infra.yasm.yasmapi import GolovanRequest


logging.basicConfig(
    level=logging.INFO,
    format="[%(filename)s:%(lineno)d] %(levelname)-8s [%(asctime)s]  %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
)

SECONDS_IN_DAY = 3600 * 24
SOLOMON_URL = "https://solomon.yandex.net"


@retry.retry(tries=5, delay=1, backoff=2, max_delay=60)
def get_hosts():
    response = requests.get("https://yasm.yandex-team.ru/metainfo/hosts?itype=antirobot").json()
    assert response["status"] == "ok"
    result = set(item["name"] for item in response["response"]["result"])
    return sorted(list(result))


@retry.retry(tries=7, delay=1, backoff=2, max_delay=60)
def golovan_request(host, period, ts_start, ts_end, signals):
    return list(GolovanRequest(host, period, ts_start, ts_end, signals, max_retry=5, retry_delay=1))


@click.group()
def main():
    pass


@main.command('solomon_aggr')
@click.option('--days-ago', type=int, default=1)
@click.option('--days-count', type=int, default=1)
@click.option('--solomon-token', type=str, required=True)
def solomon_aggr(days_ago, days_count, solomon_token):
    now = time.time()
    for days_shift in range(days_ago, days_ago + days_count):
        for days_period in (1, 7):
            end_ts = now - SECONDS_IN_DAY * days_shift - 2 * 3600
            start_ts = end_ts - SECONDS_IN_DAY * days_period + 3600

            aggrs = [
                "havg(portoinst-cpu_usage_cores_hgram)",
                "quant(portoinst-cpu_usage_cores_hgram, 99)",
            ]
            get_key = lambda x: "itype=antirobot:" + x

            points = defaultdict(list)
            for timestamp, values in GolovanRequest("ASEARCH", 3600, start_ts, end_ts, list(map(get_key, aggrs))):
                for aggr in aggrs:
                    points[aggr].append(values[get_key(aggr)])

            values = list(map(lambda x: np.nanmean(np.array(x)), points.values()))
            client = solomon.BasePushApiReporter(project="antirobot",
                                                 cluster="cpu",
                                                 service="cpu",
                                                 url=SOLOMON_URL,
                                                 auth_provider=solomon.OAuthProvider(solomon_token))

            client.set_value(sensor=aggrs, value=values, labels=[{"aggregate_period": f"{days_period}day"}] * len(values), ts_datetime=datetime.fromtimestamp(end_ts))


@main.command('solomon_stat_by_host')
@click.option('--days-ago', type=int, default=1)
@click.option('--days-count', type=int, default=1)
@click.option('--host', type=str)
@click.option('--solomon-token', type=str, required=True)
def solomon_stat_by_host(days_ago, days_count, host, solomon_token):
    do_host_aggr = False
    if not host:
        hosts = get_hosts()
        do_host_aggr = True
    else:
        hosts = [host]

    now = time.time()

    aggr_values = defaultdict(lambda: defaultdict(int))
    solomon_client = solomon.BasePushApiReporter(project="antirobot",
                                                 cluster="cpu",
                                                 service="cpu",
                                                 url=SOLOMON_URL,
                                                 auth_provider=solomon.OAuthProvider(solomon_token))

    for host in hosts:
        logging.info(f"Process host={host}")
        for days_shift in range(days_ago, days_ago + days_count):
            end_ts = now - SECONDS_IN_DAY * days_shift - 2 * 3600
            start_ts = end_ts - SECONDS_IN_DAY + 3600

            limit_key = "min(portoinst-cpu_guarantee_cores_thhh)"
            usage_key = "quant(portoinst-cpu_usage_cores_hgram, 99)"

            aggrs = [
                limit_key,
                usage_key
            ]
            get_key = lambda x: f"itype=antirobot;ctype=prod:{x}"

            limit_values = []
            usage_values = []
            for timestamp, values in golovan_request(host, 3600, start_ts, end_ts, list(map(get_key, aggrs))):
                limit_values.append(values[get_key(limit_key)] or 0)
                usage_values.append(values[get_key(usage_key)] or 0)

            usage = np.nanmax(usage_values)
            limit = np.nanmax(limit_values)

            for p in (75, 80, 90, 95, 99):
                value = 1 if usage >= limit / 100.0 * p else 0
                solomon_client.set_value(sensor=[f"cpu_load_{p}"], value=[value], labels=[{"host": host}], ts_datetime=datetime.fromtimestamp(end_ts))
                aggr_values[end_ts][p] += value

    if do_host_aggr:
        for end_ts, d in aggr_values.items():
            for p, value in d.items():
                solomon_client.set_value(sensor=[f"cpu_load_{p}_sum"], value=[value], labels=[], ts_datetime=datetime.fromtimestamp(end_ts))


if __name__ == "__main__":
    main()
