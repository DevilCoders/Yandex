# -*- coding: utf-8 -*-
import sys
from mock import Mock

from ..base.tests import _BaseStatsAggregator


class MockedUwsgi:
    def __init__(self):
        self.cache = {}

    def cache_inc(self, key, amount):
        if key in self.cache:
            self.cache[key] += amount
        else:
            self.cache[key] = amount

    def cache_num(self, key):
        return int(self.cache.get(key, 0))

    def cache_keys(self):
        return self.cache.keys()

    def cache_update(self, key, value):
        self.cache[key] = value

    def cache_get(self, key):
        return self.cache.get(key, None)

sys.modules['uwsgi'] = MockedUwsgi()
import uwsgi

from golovan_stats_aggregator.uwsgi import UwsgiStatsAggregator


class TestUwsgiStatsAggregator(_BaseStatsAggregator):
    implementation_class = UwsgiStatsAggregator

    def get_stats_aggregator(self, left_border=0.001, right_border=1000.0, progression_step=1.5):
        uwsgi.cache = {}

        return super(TestUwsgiStatsAggregator, self).get_stats_aggregator(
            left_border=left_border,
            right_border=right_border,
            progression_step=progression_step
        )
