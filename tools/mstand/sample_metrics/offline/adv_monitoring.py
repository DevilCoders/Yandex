from metrics_api.offline import SerpMetricParamsForAPI  # noqa
from yaqlibenums import SerpComponentType


class HasAdv:
    def __init__(self):
        pass

    def value(self, metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: float
        """
        results = metric_params.results
        for index, result in enumerate(results):
            # see SerpComponentType
            if result.result_type == SerpComponentType.ADDV:
                return 1.0

        return 0.0


class AdvCount:
    def __init__(self):
        pass

    def value(self, metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: float
        """
        ad_total_count = 0.0
        results = metric_params.results
        for index, result in enumerate(results):
            # see SerpComponentType
            if result.result_type == SerpComponentType.ADDV:
                ad_total_count += 1.0

        return ad_total_count


class AdvExpCount:
    def __init__(self):
        pass

    def value(self, metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: float
        """
        ad_total_weight = 0.0
        results = metric_params.results
        current_weight = 1.0
        for index, result in enumerate(results):
            # see SerpComponentType
            if result.result_type == SerpComponentType.ADDV:
                # more adv results => higher metric value
                ad_total_weight += 100.0 * current_weight
                current_weight *= 2.0

        return ad_total_weight
