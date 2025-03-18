import math


def is_judged(scales, scale):
    return scale in scales and 'NOT_JUDGED' != scales[scale]


class ImagesRelSize:
    def __init__(self, depth=10, use_size=True, judged_only=True):
        self.depth = depth
        self.use_size = use_size
        self.judged_only = judged_only

    @staticmethod
    def rel_scale(rel_mark):
        rel_map = {
            'RELEVANT_PLUS': 1.0,
            'RELEVANT_MINUS': 0.5,
        }
        return rel_map.get(rel_mark, 0.)

    def value(self, metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: float
        """

        results = metric_params.results[:self.depth]
        if results:
            accum = 0.
            docs = 0
            for res in results:
                if self.judged_only and not is_judged(res.scales, "RELEVANCE"):
                    continue
                relevance = self.rel_scale(res.scales["RELEVANCE"])
                size_factor = 1.
                if self.use_size:
                    dimensions = res.scales.get("dimension.IMAGE_DIMENSION")
                    if dimensions and "w" in dimensions and "h" in dimensions:
                        area = dimensions["w"] * dimensions["h"]
                        size_factor = 1. / (1. + math.exp((math.sqrt(area) - 500) / -100))
                    elif self.judged_only:
                        continue
                docs += 1
                accum += relevance * size_factor
            return accum / docs if docs else 0.
