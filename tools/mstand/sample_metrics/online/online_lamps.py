import itertools

import sample_metrics.online.online_metric_helpers as omh
from experiment_pool import MetricColoring
from experiment_pool import MetricValueType
from metrics_api.online import UserActionsForAPI  # noqa


class SumUserCount(object):
    coloring = MetricColoring.LAMP
    value_type = MetricValueType.SUM

    def __init__(self):
        pass

    def __call__(self, metric_params):
        """
        :type metric_params: UserActionsForAPI
        :rtype: int
        """
        return 1


# same as TotalNumberOfRequests
class SumActionCount(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.LAMP

    def __init__(self, only_matching=False):
        self.only_matching = only_matching

    def __call__(self, metric_params):
        return len(metric_params.actions)


# count all clicks, not only by results/wizards
class SumClickCountWeb(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.LAMP

    def __init__(self):
        pass

    @staticmethod
    def click_filter(action):
        return omh.is_click(action)

    def __call__(self, metric_params):
        """
        :type metric_params: UserActionsForAPI
        :rtype: int
        """
        return sum(1 for action in metric_params.actions if self.click_filter(action))


# count all clicks, not only by results/wizards
class SumClickCountImages(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.LAMP

    def __init__(self):
        pass

    @staticmethod
    def click_filter(action):
        return omh.is_img_click(action)

    def __call__(self, metric_params):
        """
        :type metric_params: UserActionsForAPI
        :rtype: int
        """
        return sum(1 for action in metric_params.actions if self.click_filter(action))


# same as TotalNumberOfClicks
class SumResultClickCount(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.LAMP

    def __init__(self, only_matching=False):
        self.only_matching = only_matching

    @staticmethod
    def click_filter(action):
        return omh.is_click(action) and omh.is_click_on_result_or_wizard(action)

    def __call__(self, metric_params):
        """
        :type metric_params: UserActionsForAPI
        :rtype: int
        """
        return sum(1 for action in metric_params.actions if self.click_filter(action))


# same as TotalNumberOfRequests
class SumRequestCount(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.LAMP

    def __init__(self):
        pass

    @staticmethod
    def request_filter(action):
        return omh.is_request(action)

    def __call__(self, metric_params):
        """
        :type metric_params: UserActionsForAPI
        :rtype: int
        """
        return sum(1 for action in metric_params.actions if self.request_filter(action))


class SumUserCountWithClicksBeforeRequest(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.LAMP

    def __init__(self):
        pass

    def __call__(self, metric_params):
        """
        :type metric_params: UserActionsForAPI
        :rtype: int
        """
        return SumUserCountWithClicksBeforeRequest.calc_user_count(metric_params.actions)

    @staticmethod
    def calc_user_count(actions):
        request_reqids = set()
        for action in actions:
            reqid = action.data.get("reqid")
            if omh.is_request(action):
                if reqid is not None:
                    request_reqids.add(reqid)
            elif omh.is_click(action):
                if reqid is not None and reqid not in request_reqids:
                    return 1
        return 0


class SumUserCountWithClicksBeforeRequestPerDay(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.LAMP

    def __init__(self):
        pass

    def __call__(self, metric_params):
        """
        :type metric_params: UserActionsForAPI
        :rtype: __generator[int]
        """
        for _, group in itertools.groupby(metric_params.actions, key=lambda act: act.date):
            yield SumUserCountWithClicksBeforeRequest.calc_user_count(group)


class SumUserCountWithClicksWithoutReqids(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.LAMP

    def __init__(self):
        pass

    def __call__(self, metric_params):
        """
        :type metric_params: UserActionsForAPI
        :rtype: int
        """
        for action in metric_params.actions:
            if omh.is_click(action):
                if action.data.get("reqid") is None:
                    return 1
        return 0


class SumUserCountWithRequestsWithoutReqids(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.LAMP

    def __init__(self):
        pass

    def __call__(self, metric_params):
        """
        :type metric_params: UserActionsForAPI
        :rtype: int
        """
        for action in metric_params.actions:
            if omh.is_request(action):
                if action.data.get("reqid") is None:
                    return 1
        return 0


class SumUserCountMatchingFilters(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.LAMP

    def __init__(self):
        pass

    def __call__(self, metric_params):
        if any(action.get("is_match") for action in metric_params.actions):
            return 1
        else:
            return 0


class AvgDayCount(object):
    value_type = MetricValueType.AVERAGE
    coloring = MetricColoring.LAMP

    def __init__(self):
        pass

    def __call__(self, metric_params):
        """
        :type metric_params: UserActionsForAPI
        :rtype: int
        """
        action_days = set()
        for action in metric_params.actions:
            action_days.add(action.date)
        return len(action_days)


class UsersWithMultipleBuckets(object):
    value_type = MetricValueType.SUM
    coloring = MetricColoring.LAMP

    def __init__(self):
        pass

    def __call__(self, metric_params):
        """
        :type metric_params: UserActionsForAPI
        :rtype: int
        """
        all_buckets = set(action.data.get("bucket") for action in metric_params.actions)
        all_buckets.discard(None)
        all_buckets.discard(-1)

        if len(all_buckets) > 1:
            return 1
