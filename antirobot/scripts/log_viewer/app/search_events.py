# -*- coding: utf-8 -*-

import sys
import re
import urllib
import datetime

from . import ytInst

import data_fetcher
import log_mgrs
import log_eventlog
import misc
import eventlog_adapter

from antirobot.scripts.antirobot_eventlog import event


class Adjuster:
    def __init__(self, substring, token):
        self.reGrepString = re.compile(substring) if substring else None
        self.token = token

    def __call__(self, pbEvent):
        if not any((self.reGrepString, self.token)):
            return True

        event = eventlog_adapter.MakeEventAdapter(pbEvent)

        if not event:
            return False

        if self.token and event.Token() != self.token:
            return False

        if self.reGrepString and self.reGrepString.search(event.TextData()) == None:
            return False

        return True


class EventlogFetcher(data_fetcher.DataFetcher):
    def __init__(self, conf, key, substring=None, token=None):
        logName = log_eventlog.LOG_ID
        super(EventlogFetcher, self).__init__(log_mgrs.GetPrecalcMgr(logName, key), conf.CLUSTER_ROOT, ytInst)
        self.adjuster = Adjuster(substring, token)

    def _TransformRow(self, row):
        s = row['event']
        return event.Event(s)

    def _GetAdjuster(self):
        return self.adjuster

