#!/usr/bin/python
# -*- coding: utf-8 -*-

from experiment_pool import MetricColoring
from experiment_pool import MetricValueType


class ActionCounter(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self):
        pass

    def __call__(self, actions):
        return len(actions.actions)


class ActionCounterUser(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.MORE_IS_BETTER
    user_filter = "user"

    def __init__(self):
        pass

    def __call__(self, actions):
        return len(actions.actions)


class ActionCounterDay(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.MORE_IS_BETTER
    user_filter = "day"

    def __init__(self):
        pass

    def __call__(self, actions):
        return len(actions.actions)


class ActionCounterFromFirstDay(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.MORE_IS_BETTER
    user_filter = "from_first_day"

    def __init__(self):
        pass

    def __call__(self, actions):
        return len(actions.actions)


class ActionCounterFromFirstTS(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.MORE_IS_BETTER
    user_filter = "from_first_ts"

    def __init__(self):
        pass

    def __call__(self, actions):
        return len(actions.actions)


class ActionCounterHistory(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self):
        pass

    def __call__(self, actions):
        return len(actions.history)


class ActionCounterFuture(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self):
        pass

    def __call__(self, actions):
        return len(actions.future)
