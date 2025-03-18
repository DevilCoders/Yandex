from experiment_pool import MetricColoring
from metrics_api.offline import SerpMetricParamsForAPI  # noqa


class GetProductionMetric:
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self, metric_name):
        """
        :type metric_name: str
        """
        self.metric_name = metric_name

    def value(self, metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: None
        """
        return metric_params.get_serp_data(self.metric_name)
