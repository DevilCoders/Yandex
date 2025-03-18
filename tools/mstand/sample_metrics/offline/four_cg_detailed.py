from metrics_api.offline import SerpMetricParamsByPositionForAPI  # noqa
from metrics_api.offline import SerpMetricAggregationParamsForAPI  # noqa


class FourCGDetailed:
    def __init__(self, judged=False, depth=5, rel_weight=0.5, tw_weight=0.05):
        self.judged = judged
        self.depth = depth
        self.rel_weight = rel_weight
        self.tw_weight = tw_weight
        self.counter = 0

    def scale_stat_depth(self):
        return self.depth

    def rel_scale(self, rel_mark):
        rel_map = {
            'VITAL': 1,
            'USEFUL': 0.75,
            'RELEVANT_PLUS': 0.5,
            'RELEVANT_MINUS': 0.25,
            'IRRELEVANT': 0,
            'SOFT_404': 0,
            '_404': 0,
            'VIRUS': 0,
            'NOT_JUDGED': None,
            None: 0
        }

        return rel_map.get(rel_mark, 0)

    def value_by_position(self, pos_metric_params):
        """
        :type pos_metric_params: SerpMetricParamsByPositionForAPI
        :rtype:
        """
        res = pos_metric_params.result
        index = pos_metric_params.index

        if index == 0:
            self.counter = 0

        if self.counter >= self.depth:
            return None

        rel_val = self.rel_scale(res.get_scale("RELEVANCE"))
        if self.judged and rel_val is None:
            return None
        elif rel_val is None:
            rel_val = 0

        tw_str = res.get_scale("FIVE_CG_TW_FACTOR")
        if tw_str is None:
            if self.judged:
                return None
            tw_val = 0.5
        else:
            tw_val = float(tw_str)

        dups_discount = 1

        if res.has_scale("DUPLICATE"):
            dups_discount = 0.75 ** res.get_scale("DUPLICATE")[:index].count("1")

        if res.has_scale("DUPLICATE_FULL_TOLOKA") and res.get_scale("DUPLICATE_FULL_TOLOKA")[:index].count("1") > 0:
            dups_discount = 0

        self.counter += 1
        return dups_discount * (self.rel_weight * rel_val + self.tw_weight * tw_val)

    @staticmethod
    def aggregate_by_position(agg_metric_params):
        """
        :type agg_metric_params: SerpMetricAggregationParamsForAPI
        :rtype: float
        """
        res = 0
        counter = 0
        for x in agg_metric_params.pos_metric_values:
            if x.value is not None:
                counter += 1
                res += float(x.value) / counter

        return res
