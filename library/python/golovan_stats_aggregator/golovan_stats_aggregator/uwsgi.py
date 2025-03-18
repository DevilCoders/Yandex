# -*- coding: utf-8 -*-
from __future__ import absolute_import

import re

import six
from six.moves import cPickle as pickle
import uwsgi

from golovan_stats_aggregator.base import (
    BaseStatsAggregator,
    is_validate_metric_name,
)

"""
Этот класс работает только внутри приложения под uwsgi. В противном случае, его импорт упадет с ImportError,
т.к. не будет модуля uwsgi
"""

HGRAM_METRIC_NAME_END_REGEX = re.compile(
    r'^(.+_hgram)(_\d+)$'
)


class UwsgiStatsAggregator(BaseStatsAggregator):

    def _load_pickled_value(self, key):
        value = uwsgi.cache_get(key)
        if value:
            flag, v = pickle.loads(value)
            if flag == 1:
                return v
        else:
            return 0
        raise ValueError

    def _cache_set(self, key, value):
        if not isinstance(value, six.integer_types):
            value = pickle.dumps([1, value], pickle.HIGHEST_PROTOCOL)
        uwsgi.cache_update(key, value)

    def _cache_get(self, key):
        try:
            value = self._load_pickled_value(key)
        except Exception:
            value = uwsgi.cache_num(key)
        return value

    def _cache_inc(self, key, amount):
        uwsgi.cache_inc(key, amount)

    def get_num_metric_names(self):
        keys = uwsgi.cache_keys()
        if six.PY3:
            key_names = []
            for key in keys:
                if isinstance(key, bytes):
                    key = key.decode('utf-8')
                key_names.append(key)
        else:
            key_names = keys
        return list(
            filter(is_validate_metric_name, key_names)
        )

    def get_bucket_metric_names(self):
        keys = uwsgi.cache_keys()
        result = []
        for key in keys:
            if six.PY3 and isinstance(key, bytes):
                key = key.decode('utf-8')
            tmp = HGRAM_METRIC_NAME_END_REGEX.sub(r'\1', key)
            if tmp.endswith('_hgram'):
                result.append(tmp)
        return result
