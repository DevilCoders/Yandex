# coding=utf-8

from . import online_metric_helpers as mh

from experiment_pool import MetricValueType
from experiment_pool import MetricColoring


# доля некликнутых в top3
class WebNoTop3Clicks(object):
    value_type = MetricValueType.AVERAGE
    coloring = MetricColoring.LESS_IS_BETTER

    def __init__(self):
        pass

    def __call__(self, experiment):
        requests_with_top_clicks = set()
        all_requests = set()
        for action in experiment.actions:
            if mh.is_web_servicetype(action):
                reqid = action.data["reqid"]
                all_requests.add(reqid)
                if mh.is_click(action) and mh.is_click_on_result_or_wizard(action):
                    pos = action.data["pos"]
                    if pos and int(pos) < 3:
                        requests_with_top_clicks.add(reqid)

        if all_requests:
            top_clicked_reqs = len(requests_with_top_clicks)
            all_reqs = len(all_requests)
            return 100.0 * float(all_reqs - top_clicked_reqs) / float(all_reqs)
