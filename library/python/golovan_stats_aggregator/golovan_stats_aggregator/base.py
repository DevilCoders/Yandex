# -*- coding: utf-8 -*-
import re
import time
import logging
import math
from contextlib import contextmanager
from six.moves import reduce
import six

logger = logging.getLogger('golovan_stats_aggregator')

# https://wiki.yandex-team.ru/golovan/tagsandsignalnaming/
# для гистограмм валиден только hgram
# cуффикс для сигналов, полученных с помощью стат-ручки, должен начинаться с буквы a или d (не работает для других схем сбора данных).
TAG_NAME_PATTERN = r'[a-z][0-9a-z_]{1,31}'
TAG_VALUE_PATTERN = r'[a-zA-Z0-9\.@_][a-zA-Z0-9\.\-@_]{0,127}'
METRIC_NAME_PATTERN = r'[a-zA-Z0-9\.\-/@_]{1,128}_((a|d)[vemntx]{3}|summ|hgram|max)'
FULL_METRIC_NAME_PATTERN = r'^({tag_name}={tag_value};)*{metric_name}$'.format(
    tag_name=TAG_NAME_PATTERN,
    tag_value=TAG_VALUE_PATTERN,
    metric_name=METRIC_NAME_PATTERN,
)
METRIC_NAME_REGEX = re.compile(FULL_METRIC_NAME_PATTERN)


def is_validate_metric_name(metric_name):
    return bool(METRIC_NAME_REGEX.match(metric_name))


class BaseStatsAggregator(object):
    """
    Класс сбора статистики для отправки в голован. Т.к. у нас несколько процессов, а собирать данные хочется со всех,
    нужно какое-то общее место куда будут складываться метрики. Для этого используем кэш uwsgi, который общий для всех воркеров.
    Раз в 5 секунд агент голована приходит в нашу ручку и забирает данные.

    Если нужно собирать какую-то кастомную информацию, то нужно передать собирающую функцию в метод add_metric_func.
    Она будет вызываться с некоторым интервалом и должна возвращать список метрик:
        [['my_metric_summ', 1], ['my_metric_2_annn', 10]]
    """

    def __init__(self, left_border=0.001, right_border=1000, progression_step=1.5):
        self._q = progression_step

        self._metric_funcs = []

        self._define_borders(left_border, right_border, progression_step)

    def _define_borders(self, left_border, right_border, q):
        self._pow_left = int(math.log(left_border, q))
        self._min_hgram_value = q ** self._pow_left

        pow_right = int(math.log(right_border, q) + 0.5)

        # pre-generate all the buckets
        self._buckets_range = [(0, 0)] + list(
            enumerate(
                reduce(lambda acc, _: acc + [acc[-1] * q],
                       list(range(self._pow_left + 1, pow_right + 1)),
                       [self._min_hgram_value]),
                start=1)
        )

    def _get_data_from_cache(self):
        """
        Получаем накопленную (с момента последнего сбора) информацию из кэша
        """
        stats_data = []
        for key in self.get_num_metric_names():
            value = self._cache_get(key)
            if value is None:
                value = 0
            stats_data.append([key, value])

        return stats_data

    def _validate_metric_name(self, metric_name):
        if not is_validate_metric_name(metric_name):
            raise ValueError('Invalid metric name')

    def _build_buckets_stats(self):
        """
        Строим список бакетов со значениями
        Для каждого из названий в self.metric_names (type=='bucket') идем последовательно до self._buckets_count + 1
        и строим бакет в виде: [правая_граница, вес]
        В результате у нас получается список вида: [[0, 1], [0.5, 12], [1.0, 4] ...],
        в котором представлено сколько значений у нас было от 0 до 0.5 (1), от 0.5 до 1.0 (12) и т.д.
        """
        stats = {}

        cache_get = lambda i, bucket_name: self._cache_get('{}_{}'.format(bucket_name, i))

        for bucket_name in self.get_bucket_metric_names():
            stats[bucket_name] = reduce(lambda xs, t:
                                        # t[0] - current index, t[1] - bucket border
                                        xs + [(t[1], cache_get(t[0], bucket_name))],
                                        self._buckets_range, [])

        return list(stats.items())

    def _get_index_for_value(self, value):
        """
        Получаем номер бакета которому нужно увеличить вес для данного числового значения
        """
        if value < self._min_hgram_value:
            return 0
        return int(math.log(value / self._min_hgram_value, self._q)) + 1

    @contextmanager
    def log_work_time(self, metric_name):
        now = time.time()
        yield
        work_time = time.time() - now
        try:
            self.add_to_bucket(metric_name='%s_work_time' % metric_name, value=work_time)
        except Exception:
            logger.exception('Can\'t write work time for golovan graphs!')

    def add_metric_func(self, func):
        self._metric_funcs.append(func)

    def get_data(self):
        stats_data = []

        methods = [self._get_data_from_cache, self._build_buckets_stats] + self._metric_funcs

        for method in methods:
            stats_data += method()

        return stats_data

    def add_to_bucket(self, metric_name, value):
        if not metric_name.endswith('_hgram'):
            metric_name = '%s_hgram' % metric_name
        self._validate_metric_name(metric_name)
        bucket_key = '%s_%s' % (metric_name, self._get_index_for_value(value))
        self._cache_inc(bucket_key, 1)

    def inc(self, metric_name, value=1):
        assert isinstance(value, six.integer_types)
        self._validate_metric_name(metric_name)
        self._cache_inc(metric_name, value)

    def set(self, metric_name, value):
        self._validate_metric_name(metric_name)
        self._cache_set(metric_name, value)

    def get(self, metric_name):
        self._validate_metric_name(metric_name)
        return self._cache_get(metric_name)

    def get_num_metric_names(self):
        raise NotImplementedError()

    def get_bucket_metric_names(self):
        raise NotImplementedError()

    def _cache_set(self, key, value):
        raise NotImplementedError()

    def _cache_get(self, key):
        raise NotImplementedError()

    def _cache_inc(self, key, amount):
        raise NotImplementedError()
