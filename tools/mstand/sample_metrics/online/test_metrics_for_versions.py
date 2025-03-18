from experiment_pool import MetricColoring
from experiment_pool import MetricValueType


class MetricMinVersions(object):
    value_type = MetricValueType.AVERAGE
    coloring = MetricColoring.MORE_IS_BETTER

    def __call__(self, experiment):
        return 1

    min_versions = {"_common": 2, "web": 8}


class MetricStrictMinVersions(object):
    value_type = MetricValueType.AVERAGE
    coloring = MetricColoring.MORE_IS_BETTER

    def __call__(self, experiment):
        return 1

    min_versions = {"_common": 125, "web": 80}
