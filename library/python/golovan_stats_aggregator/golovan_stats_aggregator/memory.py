# -*- coding: utf-8 -*-
import re
from collections import defaultdict
from golovan_stats_aggregator.base import BaseStatsAggregator

HGRAM_BACKET_KEY_REGEXP = re.compile(
    r'.+_hgram_\d+$'
)


def is_backet_metric_name(metric_name):
    return bool(HGRAM_BACKET_KEY_REGEXP.match(metric_name))


class MemoryStatsAggregator(BaseStatsAggregator):

    def __init__(self, left_border=0.001, right_border=1000, progression_step=1.5):
        super(MemoryStatsAggregator, self).__init__(left_border, right_border, progression_step)
        self._cache = defaultdict(float)
        self._bucket_keys = set()
        self._num_keys = set()

    def _add_to_keys(self, key):
        if is_backet_metric_name(key):
            key, _ = key.rsplit('_', 1)
            self._bucket_keys.add(key)
        else:
            self._num_keys.add(key)

    def get_bucket_metric_names(self):
        return list(self._bucket_keys)

    def get_num_metric_names(self):
        return list(self._num_keys)

    def _cache_set(self, key, value):
        self._add_to_keys(key)
        self._cache[key] = value

    def _cache_inc(self, key, amount):
        self._add_to_keys(key)
        self._cache[key] += amount

    def _cache_get(self, key):
        return self._cache.get(key, 0)
