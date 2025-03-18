# coding=utf-8

from . import online_metric_helpers as mh

from experiment_pool import MetricColoring
from experiment_pool import MetricValueType


# общее количество кликов
class TotalNumberOfClicks(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.NONE

    def __init__(self, only_matching=False):
        self.only_matching = only_matching

    def click_filter(self, action):
        is_click = mh.is_click(action) and mh.is_click_on_result_or_wizard(action)

        if self.only_matching:
            return action.get("is_match") and is_click
        else:
            return is_click

    def __call__(self, experiment):
        return sum(1 for action in experiment.actions if self.click_filter(action))
