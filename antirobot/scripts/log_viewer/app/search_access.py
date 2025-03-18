# -*- coding: utf-8 -*-

import sys
import os
import re
import pickle
import subprocess

import log_mgrs
import misc
from . import ytInst

import log_accesslog
import log_market
import data_fetcher


class Adjuster:
    @staticmethod
    def Test(dataItem, testFunc):
        for v in dataItem.values():
            unquoted = misc.TryUnquotePlus(str(v).encode('utf-8'))
            if testFunc(unquoted):
                return True

        return False

    def __init__(self, substring, isRe):
        self.substring = substring
        self.regexp = re.compile(substring) if substring and isRe else None

    def __call__(self, dataItem):
        if not self.substring:
            return True

        if self.regexp:
            return self.Test(dataItem, self.regexp.search)

        return self.Test(dataItem, lambda x: x.find(self.substring) >= 0)


class AccesslogFetcher(data_fetcher.DataFetcher):
    def __init__(self, conf, key, what, substring=None, isRe=False):

        logName = log_accesslog.LOG_ID if what == 'main' else log_market.LOG_ID
        super(AccesslogFetcher, self).__init__(log_mgrs.GetPrecalcMgr(logName, key), conf.CLUSTER_ROOT, ytInst)
        self.what = what
        self.adjuster = Adjuster(substring, isRe)

    def _GetAdjuster(self):
        return self.adjuster

