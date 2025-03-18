# coding=utf-8

from . import online_metric_helpers as mh
from experiment_pool import MetricColoring
from experiment_pool import MetricValueType


class NoNumericValues(object):
    value_type = MetricValueType.AVERAGE
    coloring = MetricColoring.NONE

    def __init__(self):
        pass

    def __call__(self, _):
        yield "MSTAND-772"
        yield u"Вот такое значение метрики"


class NoValues(object):
    value_type = MetricValueType.AVERAGE
    coloring = MetricColoring.NONE

    def __init__(self):
        pass

    def __call__(self, _):
        return


class SimpleMetricDump(object):
    value_type = MetricValueType.AVERAGE
    coloring = MetricColoring.NONE

    def __init__(self):
        pass

    def __call__(self, experiment):
        experiment.log()
        yield experiment.dump()


class SessionsWithSplit(object):
    value_type = MetricValueType.AVERAGE
    coloring = MetricColoring.NONE

    def value_by_session(self, session):
        return len(session.actions)

    def split_sessions(self, actions):  # split into two categories:
        actions_hasmisspell = []  # hasmisspell or not
        actions_nomisspell = []
        for action in actions:
            if not action.data.get("hasmisspell") or not action.data["hasmisspell"]:
                actions_nomisspell.append(action)
            else:
                actions_hasmisspell.append(action)
        return [actions_hasmisspell, actions_nomisspell]


class MetricForUsers(object):
    value_type = MetricValueType.AVERAGE
    coloring = MetricColoring.NONE

    use_buckets = False

    def value_by_user(self, experiment):
        return experiment.actions[0].user


class SessionsWOSplit(object):
    value_type = MetricValueType.AVERAGE
    coloring = MetricColoring.NONE

    use_buckets = True

    def value_by_session(self, session):
        return session.actions[0].timestamp


class AmbiguousMetric(object):
    value_type = MetricValueType.AVERAGE
    coloring = MetricColoring.NONE

    def __call__(self, _):
        pass

    def value_by_session(self, _):
        pass


# доля некликнутых в каждом запросе, это не совсем webabandonement
class RequestMetric(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.LESS_IS_BETTER

    def value_by_request(self, requests):
        requests_with_clicks = set()
        all_requests = len(requests.actions)
        for action in requests.actions:
            if mh.is_web_servicetype(action):
                reqid = action.data.get("reqid")
                if mh.is_click(action) or mh.is_dynamic_click(action):
                    requests_with_clicks.add(reqid)
        if all_requests:
            clicked_reqs = len(requests_with_clicks)
            return 100.0 * float(all_requests - clicked_reqs) / float(all_requests)


class MetricWithFiles(object):
    def list_files(self):
        return ["test1.txt"]

    def __call__(self, experiment):
        return 0
