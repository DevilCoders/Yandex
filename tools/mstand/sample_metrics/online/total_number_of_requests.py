# coding=utf-8

from . import online_metric_helpers as mh

from experiment_pool import MetricColoring
from experiment_pool import MetricValueType


# общее количество запросов
class TotalNumberOfRequests(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.NONE

    def __init__(self, only_matching=False):
        self.only_matching = only_matching

    def request_filter(self, action):
        is_request = mh.is_request(action)
        if self.only_matching:
            return action.get("is_match") and is_request
        else:
            return is_request

    def __call__(self, experiment):
        requests_number = sum(1 for action in experiment.actions if self.request_filter(action))
        if requests_number:
            return requests_number
