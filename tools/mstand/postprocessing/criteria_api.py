from experiment_pool.metric_result import MetricResult  # noqa


class CriteriaParamsForAPI(object):
    def __init__(self, control_data, exp_data, control_result, exp_result, is_related):
        """
        :type control_data: list[float] | numpy.array
        :type exp_data: list[float] | numpy.array
        :type control_result: MetricResult
        :type exp_result: MetricResult
        :type is_related: bool
        """
        if is_related and len(control_data) != len(exp_data):
            raise Exception("Cannot calc 'related' criteria for arrays of inequal length. ")
        self.control_data = control_data
        self.exp_data = exp_data
        self.control_result = control_result
        self.exp_result = exp_result
        self.is_related = is_related
