#!/usr/bin/python
# -*- coding: utf-8 -*-

from django.core.cache import cache
from snipmetricsview.utils import MetricsException

StatsTypes = {'meanStatsType' : "mean", "medianStatsType" : "median", 'stdStatsType' : "std", 'minStatsType' : "min", 'maxStatsType' : "max", 'confIntStatsType' : "confInt", "histStatsType" : "hist" }

class MetricsCache():
    def __init__(self, requestDict):
        self.keyPrefix = self.getCacheKeyPrefix(requestDict)

    @staticmethod
    def getCacheKeyPrefix(requestDict):
        if not requestDict.has_key("queryCharacteristics") or not requestDict.has_key("withTitle") or not requestDict.has_key("queryWordsCount"):
            raise MetricsException("Wrong dictionary passed as a parameter.")
        res = "view_metric_"
        res += "_".join(requestDict["queryCharacteristics"])
        res += "_".join(requestDict["queryWordsCount"])
        res += "_" + str(requestDict["withTitle"]) + "__"
        return res

    def getCacheKeyName(self, dumpId, metricId, statsType):
        return self.keyPrefix + "_"  + str(dumpId) + "_" + str(metricId) + "_" + str(statsType)

    def isInCache(self, dumpId, metricId, statsType = StatsTypes["meanStatsType"]):
        return cache.get(self.getCacheKeyName(dumpId, metricId, statsType), "FAIL") != "FAIL"

    def isAllInCache(self, dumpId, metricIds, statsType = StatsTypes["meanStatsType"]):
        res = True
        for metric in metricIds:
            res = res and self.isInCache(dumpId, metric, statsType)
        return res

    def getValue(self, dumpId, metricId, statsType):
        val = cache.get(self.getCacheKeyName(dumpId, metricId, statsType), "FAIL")
        return val if val != "FAIL" else nan

    def setValue(self, dumpId, metricId, statsType, val):
        cache.set(self.getCacheKeyName(dumpId, metricId, statsType), val)

    def mergeCacheWithData(self, dumpId, metricId, metricName, statsType, vals):
        if self.isInCache(dumpId, metricId, statsType):
            vals[metricName] = self.getValue(dumpId, metricId, statsType)
        elif vals.has_key(metricName):
            self.setValue(dumpId, metricId, statsType, vals[metricName])

    @staticmethod
    def clear():
        cache._cache.clear()
        cache._expire_info.clear()

if __name__ == "__main__":
    pass
