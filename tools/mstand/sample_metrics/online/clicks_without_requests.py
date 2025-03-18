# coding=utf-8

from . import online_metric_helpers as mh
from experiment_pool import MetricValueType
from experiment_pool import MetricColoring


# клики без запросов (краевой случай - запрос раньше начала эксперимента)
class ClicksWithoutRequests(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.NONE

    def __init__(self):
        pass

    def __call__(self, experiment):
        reqids_by_clicks = set()
        reqids_by_requests = set()
        for action in experiment.actions:
            reqid = action.data["reqid"]
            if mh.is_click(action):
                reqids_by_clicks.add(reqid)
            if mh.is_request(action):
                reqids_by_requests.add(reqid)

        diff = reqids_by_clicks - reqids_by_requests
        return len(diff)
