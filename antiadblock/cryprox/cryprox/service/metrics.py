# -*- coding: utf8 -*-
from collections import Counter, defaultdict, OrderedDict, deque
from functools import partial
from time import time

import numpy as np

from antiadblock.cryprox.cryprox.config.service import METRIC_GROUPING_TIME, TO_CLEAR_SENSORS, METRICS_TRANSFER_DELAY
from antiadblock.cryprox.cryprox.service.pipeline import EventType


# Мы не записываем продолжительности action-ов, а фиксируем лишь попадание в определенный временной интервал.
# Список верхних границ интервалов (10 ms - 5 s):
DURATION_BINS = sum([
    range(10, 100, 10),  # 10 mks - 100 mks
    range(100, 1000, 50),  # 100 mks - 1 ms
    range(1000, 10000, 1000),  # 1 ms - 10 ms
    range(10000, 500000, 10000),  # 10 ms - 500 ms
    range(500000, 1000000, 100000),  # 500 ms - 1 s
    range(1000000, 5000001, 500000),  # 1 s - 5 s (=inf)
], [])
NUMBER_OF_BINS = len(DURATION_BINS)
PERCENTILES_TO_CALC = (50, 75, 95, 98, 99)
SYSTEM_METRICS_TIME_WINDOW_SIZE = 5
# set of double 0-level domains
DOUBLE_ROOTDOMAINS = {'msk.ru', 'com.ua', 'com.am', 'com.ge', 'com.tr', 'co.il'}


def get_bucket_index(duration):
    ''' Возвращает верхнюю границу интервала времени, в который попала конкретная продолжительность duration:
    >>> DURATION_BINS[get_bucket_index(9999)]
    10000
    >>> DURATION_BINS[get_bucket_index(0)]
    10
    >>> DURATION_BINS[get_bucket_index(1000000000)]
    5000000
    '''
    lo, hi = 0, NUMBER_OF_BINS
    while lo < hi:
        mid = (lo + hi) / 2
        if DURATION_BINS[mid] < duration:
            lo = mid + 1
        else:
            hi = mid
    return min(lo, NUMBER_OF_BINS - 1)


def get_weighted_percentile(percentiles, weights, values=DURATION_BINS):
    '''
    >>> get_weighted_percentile(weights=[1,1,1], values=[1,2,3], percentiles=[0, 50, 75, 100])
    [1.0, 1.5, 2.25, 3.0]
    '''
    amount_of_values = np.sum(weights)
    if not amount_of_values:
        return [0] * len(percentiles)
    p = np.cumsum(weights).astype(float) * 100. / amount_of_values
    return np.interp(percentiles, p, values).tolist()


# https://st.yandex-team.ru/ANTIADB-1839
def format_http_host(http_host):
    '''
    >>> format_http_host("24smi.org")
    '24smi.org'
    >>> format_http_host("2a02:6b8:c08:1a85:10d:ebd7:0:6087")
    '2a02:6b8:c08:1a85:10d:ebd7:0:6087'
    >>> format_http_host("--------hearts.livejournal.com")
    'livejournal.com'
    >>> format_http_host("mineralnie-vodi.gorodrabot.ru")
    'gorodrabot.ru'
    >>> format_http_host("andreydovbenko.propartner.com.ua")
    'propartner.com.ua'
    '''
    parts = http_host.rsplit('.', 3)
    top_level_domain = '.'.join(parts[-2:])
    if top_level_domain in DOUBLE_ROOTDOMAINS:
        top_level_domain = '.'.join(parts[-3:])
    return top_level_domain


class SystemMetrics:
    """
    Stores basic overall system metrics such as cpu load percent and RPS of magnificent external server requests
    metrics are available via get_* methods
    appending of metrics is possible via append_* methods
    """

    def __init__(self):
        self.rps = deque(maxlen=SYSTEM_METRICS_TIME_WINDOW_SIZE)
        self.cpu_load_data = deque(maxlen=SYSTEM_METRICS_TIME_WINDOW_SIZE)

    @staticmethod
    def __append_ts_data(ts, _deque, key, data):
        record = _deque[-1] if _deque else None
        if record and record['ts'] == ts:
            record[key] += data
        else:
            _deque.append({'ts': ts, key: data})

    def append_cpu_load(self, ts, cpu_load_percent):
        self.__append_ts_data(ts, self.cpu_load_data, 'cpu', [cpu_load_percent])

    def append_request(self, ts):
        self.__append_ts_data(ts, self.rps, 'requests', 1)

    @staticmethod
    def __filter_ts_data(data, key):
        ts_high = int(time())
        ts_low = ts_high - SYSTEM_METRICS_TIME_WINDOW_SIZE
        return [rec[key] for rec in data if ts_low <= rec['ts'] < ts_high]

    def get_avg_rps(self):
        data = self.__filter_ts_data(self.rps, 'requests')
        return np.mean(data) if data else 0

    def get_avg_cpu_load(self):
        data = self.__filter_ts_data(self.cpu_load_data, 'cpu')
        return np.mean(np.hstack(data)) if data else 0


class AppMetrics:
    '''
    Stores app process metrics in a structure like:
    {
        <Unix timestamp>: {
            counter: collections.Counter(),  # for calculating cryprox actions per second
            duration_hist: {<action and its parameters>: np.array(NUMBER_OF_BINS), ...}  # for calculationg action procentiles
        },
        <Unix timestamp>:
        ...
    }
    '''
    metrics = defaultdict(lambda: dict(counter=Counter(), duration_hist=defaultdict(partial(np.zeros, NUMBER_OF_BINS, int))))

    def __init__(self, service_id='Unknown'):
        self.service_id = service_id

    def get_tuple_key(self, action, **kwargs):
        return (('service_id', self.service_id), ('action', action)) + tuple((k, kwargs[k]) for k in sorted(kwargs))

    def increase_counter(self, action, **kwargs):
        '''
        Used for calculating total number of actions (if we need its RPS).
        '''
        label_items = (('sensor', 'action_counter'),) + self.get_tuple_key(action, **kwargs)
        ts = int(time())
        ts -= ts % METRIC_GROUPING_TIME
        self.metrics[ts]['counter'][label_items] += 1

    def increase_histogram(self, action, duration, **kwargs):
        '''
        Used for calculating procentiles of the action.
        '''
        label_items = (('sensor', 'action_percentile'),) + self.get_tuple_key(action, **kwargs)
        ts = int(time())
        ts -= ts % METRIC_GROUPING_TIME
        self.metrics[ts]['duration_hist'][label_items][get_bucket_index(duration)] += 1

    def increase(self, action, duration, **kwargs):
        self.increase_counter(action, **kwargs)
        self.increase_histogram(action, duration, **kwargs)

    def increase_histogram_by_dimensions(self, action, duration, **kwargs):
        """
        Solomon не может обрабатывать большие количества временных рядов, поэтому для метрик с процентилями
        бесполезно делать сенсоры с большой детализацией.
        Считаем процентили по каждому разрезу на наших хостах.
        """
        self.increase_histogram(action, duration)
        for k, v in kwargs.items():
            self.increase_histogram(action, duration, **{k: v})

    @staticmethod
    def send_metrics_by_queue(pipeline):
        if not AppMetrics.metrics:
            return
        pipeline.put(EventType.SOLOMON_METRICS, dict(AppMetrics.metrics))
        AppMetrics.metrics = defaultdict(lambda: dict(counter=Counter(), duration_hist=defaultdict(partial(np.zeros, NUMBER_OF_BINS, int))))


class MetricsAggregator:

    def __init__(self):
        self.metrics = defaultdict(lambda: dict(counter=Counter(), duration_hist=defaultdict(partial(np.zeros, NUMBER_OF_BINS, int))))

    def aggregate(self, worker_metrics):
        for ts, metrics_state in worker_metrics.iteritems():
            self.metrics[ts]['counter'] += metrics_state['counter']
            for metric, weigths in metrics_state['duration_hist'].iteritems():
                self.metrics[ts]['duration_hist'][metric] += weigths
        if TO_CLEAR_SENSORS:
            ts_min = int(time()) - 2 * METRIC_GROUPING_TIME - 2 * METRICS_TRANSFER_DELAY
            for ts in self.metrics.keys():
                if ts <= ts_min:
                    del self.metrics[ts]

    def get_solomon_sensors(self):
        sensors_timeseries = defaultdict(OrderedDict)
        ts_now = int(time()) - 2 * METRICS_TRANSFER_DELAY
        sensor_ts = ts_now - ts_now % METRIC_GROUPING_TIME - METRIC_GROUPING_TIME
        for ts in self.metrics:
            if TO_CLEAR_SENSORS:
                if ts != sensor_ts:
                    continue
            for label_items, value in self.metrics[ts]['counter'].iteritems():
                sensors_timeseries[label_items][ts] = float(value) / METRIC_GROUPING_TIME  # actions/s
            for label_items, bin_weights in self.metrics[ts]['duration_hist'].iteritems():
                for percent, value in zip(PERCENTILES_TO_CALC, get_weighted_percentile(PERCENTILES_TO_CALC, bin_weights)):
                    if value == 0:
                        continue  # case that means we don't have data
                    percentile_label = label_items + (('p', percent),)
                    sensors_timeseries[percentile_label][ts] = int(value)

        list_of_sensors = [{
            'labels': dict(labels),
            'timeseries': [{'ts': ts, 'value': value} for ts, value in sorted(timeseries.items(), key=lambda x: x[0])],
        } for labels, timeseries in sensors_timeseries.items()]
        return list_of_sensors


SOLOMON_METRICS_AGG = MetricsAggregator()
SYSTEM_METRICS = SystemMetrics()
