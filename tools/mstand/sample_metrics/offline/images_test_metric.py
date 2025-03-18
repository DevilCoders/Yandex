from metrics_api.offline import SerpMetricParamsForAPI  # noqa
import yaqutils.math_helpers as umath


class ImagesTest:
    def __init__(self):
        pass

    @staticmethod
    def value(metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: float
        """
        if metric_params.results:
            vals = []
            for res in metric_params.results:
                dimensions = res.scales.get("dimension.IMAGE_DIMENSION")
                if dimensions:
                    vals.append(dimensions["w"] * dimensions["h"])
            if vals:
                return umath.avg(vals)
            else:
                return 0.0
