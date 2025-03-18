from experiment_pool import MetricColoring
from experiment_pool import MetricValueType


class MetricRequiredServices(object):
    value_type = MetricValueType.AVERAGE
    coloring = MetricColoring.MORE_IS_BETTER

    def __call__(self, experiment):
        return 1

    required_services = ["web"]


class MetricMoreRequiredServices(object):
    value_type = MetricValueType.AVERAGE
    coloring = MetricColoring.MORE_IS_BETTER

    def __call__(self, experiment):
        return 1

    required_services = ["alice", "taxi", "web"]
