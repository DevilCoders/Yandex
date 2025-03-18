class YtCalcTestDetailed:
    def __init__(self):
        pass

    @staticmethod
    def value_by_position(params):
        """
        :type params:
        :return:
        """
        index = params.index
        return index

    @staticmethod
    def aggregate_by_position(metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: float
        """
        return sum([x.value for x in metric_params.pos_metric_values])


class YtCalcTest:
    def __init__(self):
        pass

    @staticmethod
    def value(params):
        """
        :type params:
        :return:
        """
        return len(params.query_text)
