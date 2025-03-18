import time
from datetime import datetime
from operator import itemgetter

import numpy as np
import click
import requests
import logging

from infra.yasm.yasmapi import GolovanRequest
import solomon
import retry


logging.basicConfig(
    level=logging.INFO,
    format="[%(filename)s:%(lineno)d] %(levelname)-8s [%(asctime)s]  %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
)


SECONDS_IN_HOUR = 3600
SECONDS_IN_DAY = SECONDS_IN_HOUR * 24
HOURS_IN_WEEK = 24 * 7
SOLOMON_URL = "https://solomon.yandex.net"

NORMAL_RPS_HOURS = HOURS_IN_WEEK * 3
NORMAL_RPS_PERCENTILE = 95


@retry.retry(tries=11, delay=1, backoff=2, max_delay=60)
def golovan_request(period, ts_start, ts_end, signals):
    return list(GolovanRequest("ASEARCH", period, ts_start, ts_end, signals, max_retry=5, retry_delay=1))


@retry.retry(tries=5, delay=1, backoff=2, max_delay=60)
def get_service_types():
    response = requests.get("https://yasm.yandex-team.ru/metainfo/tags?itype=antirobot").json()
    assert response["status"] == "ok"
    result = set(item["service_type"] for item in response["response"]["result"])
    result.remove("")
    return sorted(list(result))


@click.group()
def main():
    pass


class SolomonBatchedSender:
    def __init__(self, solomon_token, cluster, service, service_type, timestamp, prefix=''):
        self.timestamp = timestamp
        self.service_type = service_type
        self.prefix = prefix
        self.client = solomon.BasePushApiReporter(project="antirobot",
                                                  cluster=cluster,
                                                  service=service,
                                                  url=SOLOMON_URL,
                                                  auth_provider=solomon.OAuthProvider(solomon_token))
        self.sensors = []
        self.values = []
        self.labels = []

    def add(self, sensor, value):
        self.sensors.append(self.prefix + sensor)
        self.values.append(value)
        self.labels.append({"service_type": self.service_type})

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.client.set_value(sensor=self.sensors,
                              value=self.values,
                              labels=self.labels,
                              ts_datetime=datetime.fromtimestamp(self.timestamp))


@main.command('solomon_aggr')
@click.option('--days-ago', type=int, default=0)
@click.option('--days-count', type=int, default=1)
@click.option('--service-types', type=str, default='')
@click.option('--solomon-token', type=str, required=True)
def solomon_aggr(days_ago, days_count, service_types, solomon_token):
    if service_types:
        service_types = service_types.split(",")
    else:
        service_types = get_service_types()
    logging.info("service_types = %s" % ",".join(service_types))
    now_ts = int(time.time()) - 2*SECONDS_IN_HOUR

    for service_type in service_types:
        logging.info(f"Process service_type={service_type}")

        for days_shift in range(days_ago, days_ago + days_count):
            now = now_ts - days_shift * SECONDS_IN_DAY

            logging.info("Now is " + str(now))

            try:
                # запрашиваем статистику сразу за 3 недели + 1 день
                logging.info("Request unistat_daemon-requests_passed_to_service_deee from yasm (1h)")
                hourly_passed_all = [(timestamp, list(values.values())[0] or 0) for timestamp, values in
                                     golovan_request(SECONDS_IN_HOUR,
                                                     now - NORMAL_RPS_HOURS * SECONDS_IN_HOUR - SECONDS_IN_DAY, now,
                                                     [f"itype=antirobot;service_type={service_type}:sum(unistat_daemon-requests_passed_to_service_deee)"])]

                logging.info("Request unistat_daemon-requests_passed_to_service_deee from yasm (5min)")
                min5_passed_all = [list(values.values())[0] or 0 for timestamp, values in
                                   golovan_request(5 * 60, now - SECONDS_IN_DAY, now,
                                                   [f"itype=antirobot;service_type={service_type}:sum(unistat_daemon-requests_passed_to_service_deee)"])]

                logging.info("Request unistat_daemon-requests_deee from yasm (5min)")
                min5_requests_all = [list(values.values())[0] or 0 for timestamp, values in
                                     golovan_request(5 * 60, now - SECONDS_IN_DAY, now,
                                                     [f"itype=antirobot;service_type={service_type}:sum(unistat_daemon-requests_deee)"])]

                normal_passed = int(np.percentile(
                    list(filter(lambda x: x is not None, map(itemgetter(1), hourly_passed_all[0:NORMAL_RPS_HOURS]))),
                    NORMAL_RPS_PERCENTILE))

                if normal_passed == 0:
                    logging.info("Skip, normal_rps is 0")
                    continue

                logging.info("Push to solomon")
                passed_ratio_history = []
                requests_ratio_history = []
                for i in range(len(min5_passed_all)):
                    passed_ratio = 12.0 * min5_passed_all[i] / normal_passed
                    requests_ratio = 12.0 * min5_requests_all[i] / normal_passed

                    passed_ratio_history.append(passed_ratio)
                    requests_ratio_history.append(requests_ratio)

                timestamp = hourly_passed_all[-1][0]

                with SolomonBatchedSender(solomon_token, "rps", "rps_passed", service_type, timestamp) as solomon_sender:
                    solomon_sender.add("normal_passed", normal_passed)

                    for thr in (1.5, 1.8, 2, 3, 5):
                        solomon_sender.add(f"passed_{thr}x", (np.array(passed_ratio_history) > thr).mean())
                        for passed_thr in (1.3, 1.5, 1.7):
                            solomon_sender.add(f"passed_{passed_thr}x_{thr}x",
                                               ((np.array(passed_ratio_history) < passed_thr) &
                                                (np.array(requests_ratio_history) > thr)).mean())

            except Exception as e:
                logging.exception(e)


if __name__ == "__main__":
    main()
