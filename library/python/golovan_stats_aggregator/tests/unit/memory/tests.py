# -*- coding: utf-8 -*-
from ..base.tests import _BaseStatsAggregator

from golovan_stats_aggregator import MemoryStatsAggregator


class TestMemoryStatsAggregator(_BaseStatsAggregator):
    implementation_class = MemoryStatsAggregator
