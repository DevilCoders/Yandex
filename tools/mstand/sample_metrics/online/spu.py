# -*- coding: utf-8 -*-
from datetime import timedelta

from experiment_pool import MetricColoring
from experiment_pool import MetricValueType


class SPU(object):
    coloring = MetricColoring.MORE_IS_BETTER
    value_type = MetricValueType.AVERAGE

    def __init__(self, minutes=30):
        self._time_delta = timedelta(minutes=minutes)

    def __call__(self, experiment):
        types = {"click", "request", "dynamic-click"}
        sessions = 0
        previous = None
        for action in experiment.actions:
            if action.data.get("type") in types:
                current = action.datetime
                if previous is None or current - previous > self._time_delta:
                    sessions += 1
                previous = current
        if sessions:
            return sessions
