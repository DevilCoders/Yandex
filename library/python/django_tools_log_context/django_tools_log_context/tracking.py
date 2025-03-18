# -*- coding: utf-8 -*-
from __future__ import unicode_literals
from django.conf import settings
from . import dbtracking, httptracking


def enable_tracking():
    if settings.TOOLS_LOG_CONTEXT_ENABLE_DB_TRACKING:
        dbtracking.enable_instrumentation()
    if settings.TOOLS_LOG_CONTEXT_ENABLE_HTTP_TRACKING:
        httptracking.enable_instrumentation()


def disable_tracking():
    dbtracking.disable_instrumentation()
    httptracking.disable_instrumentation()
