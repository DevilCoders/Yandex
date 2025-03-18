from metrics_api.offline import SerpMetricParamsForAPI  # noqa
import yaqutils.math_helpers as umath

# Usage sample:
# https://nirvana.yandex-team.ru/flow/fde68441-485f-11e6-8618-0025909427cc/graph


class SerpExtendTest:
    def __init__(self):
        pass

    @staticmethod
    def value(metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: float
        """
        # In query+URL enrichment, custom fields are added to metric_params.results[i].scales
        custom_values = []
        for res in metric_params.results:
            custom = res.scales.get("custom", 0)
            if isinstance(custom, dict):
                custom_value = custom.get("line", 0)
            else:
                custom_value = custom
            custom_values.append(custom_value)

        return umath.avg(custom_values)


class SerpExtendTestQuerywise:
    def __init__(self):
        pass

    @staticmethod
    def value(metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: float
        """
        # In querywise enrichment, custom fields are added to metric_params.serp_data
        custom_value = metric_params.serp_data.get("custom", 0)
        if isinstance(custom_value, dict):
            return custom_value.get("line", 0)
        else:
            return custom_value
