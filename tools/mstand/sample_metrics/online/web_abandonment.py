# coding=utf-8

from . import online_metric_helpers as mh

from experiment_pool import MetricColoring
from experiment_pool import MetricValueType


# доля некликнутых
class WebAbandonment(object):
    value_type = MetricValueType.AVERAGE
    coloring = MetricColoring.LESS_IS_BETTER

    def __init__(self):
        pass

    def __call__(self, experiment):
        requests_with_clicks = set()
        all_requests = set()
        for action in experiment.actions:
            # TODO: only web or all?
            if mh.is_web_servicetype(action):
                reqid = action.data.get("reqid")
                all_requests.add(reqid)
                if mh.is_click(action) or mh.is_dynamic_click(action):
                    requests_with_clicks.add(reqid)
        if all_requests:
            clicked_reqs = len(requests_with_clicks)
            all_reqs = len(all_requests)
            return 100.0 * float(all_reqs - clicked_reqs) / float(all_reqs)
