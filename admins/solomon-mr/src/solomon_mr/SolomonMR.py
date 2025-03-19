#! /usr/bin/python3
import os
import sys
from collections import defaultdict
from copy import deepcopy
from numbers import Number
from typing import List

import argparse
import logging
import logging.config
import requests
import socket
import time
import yaml
from datetime import datetime
from kazoo.client import KazooClient

from src.solomon_mr.Config import Config
from src.solomon_mr.DataPoint import DataPointKV, DataPointLabels
from src.solomon_mr.Juggler import Juggler, JugglerEvent
from src.solomon_mr.SolomonDAO import SolomonDAO


class SolomonMR:
    def __init__(self, daemon_mode: bool):
        self._init_logging()
        self._log = logging.getLogger(self.__class__.__name__)
        self._daemon_mode = daemon_mode

        self._args = self._get_args()
        self._config = Config(self._args.config)
        self._solomon = SolomonDAO(self._get_token(), self._config.output)

        if daemon_mode:
            self._juggler = Juggler(os.environ['JUGGLER_TOKEN'])
            self._run_daemon()
        else:
            self._juggler = None
            self._run_recalculate()

    def _run_daemon(self) -> None:
        zk = self._get_zk()

        lock = zk.Lock(self._config.zookeeper.lock, socket.gethostname())
        while True:
            try:
                if lock.acquire(blocking=False):
                    self._log.info('Got a lock, processing')
                    now = int(datetime.now().timestamp())
                    self._process_once(now)
                else:
                    self._log.info('Lock not acquired')
                time.sleep(self._config.delay)
            finally:
                lock.release()

    def _run_recalculate(self):
        stop_ts = self._args.stop_ts
        if not stop_ts:
            stop_ts = int(datetime.now().timestamp())

        period = stop_ts - self._args.start_ts

        labels = self._config.output
        for check in self._config.check:
            self._log.info('Recalculating sensor %s', check)
            labels.sensor = check
            points = self._solomon.get_data(stop_ts, labels, period)
            for point in points:
                self._process_once(point.timestamp)
            return

    def _get_args(self):
        ap = argparse.ArgumentParser()
        ap.add_argument('config')
        if not self._daemon_mode:
            ap.add_argument('--mode', choices=['recalculate', 'set_ones'])
            ap.add_argument('--start_ts',
                            type=int,
                            help='The timestamp to work from, as in $(date +%%s --date="...")',
                            required=True)
            ap.add_argument('--stop_ts',
                            type=int,
                            help='The timestamp to work until')
        return ap.parse_args()

    @staticmethod
    def _init_logging() -> None:
        try:
            # noinspection PyUnresolvedReferences
            import library.python.resource
            resource = library.python.resource.find('logging.yaml')
        except ImportError:
            resource = open('logging.yaml').read()
        conf = yaml.full_load(resource)
        if sys.stdout.isatty() or 'PYCHARM_HOSTED' in os.environ:
            conf['loggers']['']['handlers'] = ['console']
            del conf['handlers']['file']
        logging.config.dictConfig(conf)

    @staticmethod
    def _get_token() -> str:
        """ Retrieve token either from the env or from the runtime config """
        if 'TOKEN' in os.environ:
            return os.environ['TOKEN']
        with open(os.path.expanduser('~/.runtime.yaml')) as fin:
            data = yaml.load(fin, Loader=yaml.FullLoader)
        return data['solomon_token']

    def _process_once(self, timestamp: int) -> None:
        """ Run one processing cycle """
        checks = self._calculate_checks_and_limits(timestamp)
        self._solomon.submit_data(timestamp, checks)

        availability = self._calculate_availability(timestamp)
        self._solomon.submit_data(timestamp, availability)

        if not self._daemon_mode and self._juggler:
            self._juggler.post_events(
                [JugglerEvent(host=self._config.juggler.host,
                              service=c.sensor,
                              description='{} is {} of the SLO'.format(c.sensor,
                                                                       'inside' if c.value > 0 else 'OUTSIDE'),
                              status='OK' if c.value > 0 else 'CRIT') for c in checks]
            )

    def _get_zk(self) -> KazooClient:
        """ Zookeeper initializer """
        resp = requests.get(self._config.zookeeper.url)
        resp.raise_for_status()
        url = ','.join([f'{x}:2181' for x in resp.text.strip().split()])
        self._log.info('Connecting to zk with ' + url)
        result = KazooClient(url)
        result.start()
        return result

    def _get_avg_data(self, timestamp: int, params: DataPointLabels):
        """ Get the average data from Solomon """
        items = [x.value for x in self._solomon.get_data(timestamp, params, self._config.average_over_secs)]
        if not items:
            raise KeyError()  # gets caught later in the stack
        return sum(items) / len(items)

    def _calculate_availability(self, timestamp: int) -> List[DataPointKV]:
        """ Calculate availability items based on the config """
        force_ones = not self._daemon_mode and self._args.mode == 'set_ones'
        biggest_period = max(self._config.totals.values())

        result: List[DataPointKV] = []
        total = defaultdict(int)
        total_good = defaultdict(int)
        now = int(time.time())

        for sensor in self._config.check:
            params = deepcopy(self._config.output)
            params.sensor = sensor
            data = list(self._solomon.get_data(timestamp, params, biggest_period))

            for period_name, period in self._config.totals.items():
                period_start = now - period
                data_chunk = [x.value for x in data if x.timestamp > period_start]
                chunk_len = len(data_chunk)

                if not chunk_len:
                    continue

                goods_in_chunk = 0
                for item in data_chunk:
                    if item > 0.5 or force_ones:
                        goods_in_chunk += 1

                self._log.debug('%s @ %s => %s / %s = %.2f', sensor, period_name, goods_in_chunk, chunk_len,
                                goods_in_chunk / chunk_len)
                result.append(DataPointKV(f'{sensor}-{period_name}', goods_in_chunk / chunk_len))
                total[period_name] += chunk_len
                total_good[period_name] += goods_in_chunk

        for period_name in self._config.totals:
            if total[period_name] > 0:
                self._log.debug('total @ %s => %s / %s = %.2f', period_name, total_good[period_name],
                                total[period_name], total_good[period_name] / total[period_name])
                result.append(DataPointKV(f'total-{period_name}', total_good[period_name] / total[period_name]))

        return result

    def _calculate_checks_and_limits(self, timestamp: int) -> List[DataPointKV]:
        """ Calculate check statuses based on the config and data from solomon """
        in_data = {}
        for key, params in self._config.in_data.items():
            try:
                in_data[key] = self._get_avg_data(timestamp, params)
            except KeyError:
                self._log.warning('Could not get data for {}'.format(key))

        result_data: List[DataPointKV] = []
        for key, desc in self._config.check.items():
            try:
                left = in_data[desc.left]
                if isinstance(desc.right, Number):
                    right = desc.right
                else:
                    right = in_data[desc.right] * desc.scale
                result_data.append(DataPointKV(key, 1 if left < right else 0))
                result_data.append(DataPointKV(key + '-limit', right))
            except KeyError:
                self._log.warning('Could not calculate checks for {}'.format(key))
        return result_data


def daemon_main():
    SolomonMR(True)


def recalculate_main():
    SolomonMR(False)


if __name__ == '__main__':
    recalculate_main()
