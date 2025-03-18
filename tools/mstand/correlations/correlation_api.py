# NOTICE: Classes below are used directly in user correlation functions.
# Interface/fields should NOT be changed/renamed/etc. without 'mstand community' approval.

from experiment_pool import MetricDiff  # noqa
from experiment_pool import MetricValues  # noqa


class MetricValuesForAPI(object):
    def __init__(self, metric_values):
        """
        :type metric_values: MetricValues
        """
        self.value = metric_values.significant_value
        self.count_val = metric_values.count_val
        self.sum_val = metric_values.sum_val
        self.row_count = metric_values.row_count


class MetricDiffForAPI(object):
    def __init__(self, metric_diff):
        """
        :type metric_diff: MetricDiff
        """
        self.abs_diff = metric_diff.significant.abs_diff
        self.rel_diff = metric_diff.significant.rel_diff


class CorrelationValuesForAPI(object):
    def __init__(self, control_values, exp_values, metric_diff, pvalue, deviations):
        """
        :type control_values: MetricValuesForAPI
        :type exp_values: MetricValuesForAPI
        :type metric_diff: MetricDiffForAPI
        :type pvalue: float
        :type deviations: Deviations
        """
        self.control_values = control_values
        self.exp_values = exp_values
        self.metric_diff = metric_diff
        self.pvalue = pvalue
        self.deviations = deviations

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "CorrVal(pv={}, dev={})".format(self.pvalue, self.deviations)


class CorrelationPairForAPI(object):
    def __init__(self, left, right):
        """
        :type left: CorrelationValuesForAPI
        :type right: CorrelationValuesForAPI
        """
        if not isinstance(left, CorrelationValuesForAPI):
            raise Exception("Wrong 'left' parameter in CorrelationPairForAPI, expected CorrelationValuesForAPI")
        if not isinstance(right, CorrelationValuesForAPI):
            raise Exception("Wrong 'right' parameter in CorrelationPairForAPI, expected CorrelationValuesForAPI")
        self.left = left
        self.right = right

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "Pair(l={}, r={})".format(self.left, self.right)


class CorrelationParamsForAPI(object):
    def __init__(self, corr_pairs, additional_info=""):
        """
        :type corr_pairs: list[CorrelationPairForAPI]
        :type additional_info: str
        """
        if not isinstance(corr_pairs, list):
            raise Exception("Wrong type for 'corr_pairs' parameter in CorrelationParamsForAPI")
        self.corr_pairs = corr_pairs
        self.additional_info = additional_info

    def set_additional_info(self, info):
        """
        :type info: str
        """
        if not isinstance(info, str):
            raise Exception("Wrong type for additional_info in CorrelationParamsForAPI (use str)")
        self.additional_info = info

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "CorrParams(corr_pairs={})".format(self.corr_pairs)
