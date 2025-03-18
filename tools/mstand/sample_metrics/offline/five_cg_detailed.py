from metrics_api.offline import SerpMetricParamsByPositionForAPI  # noqa
from metrics_api.offline import SerpMetricAggregationParamsForAPI  # noqa


class FiveCGDetailed:
    def __init__(self, depth=30):
        self.depth = depth
        self.rel_weight = 0.5
        self.tw_weight = 0.1
        self.click_weight = 0.4

    def scale_stat_depth(self):
        return self.depth

    def serp_depth(self):
        return self.depth

    @staticmethod
    def rel_scale(rel_mark):
        rel_map = {
            'VITAL': 1.0,
            'USEFUL': 0.75,
            'RELEVANT_PLUS': 0.5,
            'RELEVANT_MINUS': 0.25,
            'IRRELEVANT': 0,
            'SOFT_404': 0,
            '_404': 0,
            'VIRUS': 0,
            'NOT_JUDGED': 0.5,
            'AUX': 0.0,
            None: 0,
        }
        return rel_map.get(rel_mark, 0.0)

    def value_by_position(self, pos_metric_params):
        """
        :type pos_metric_params: SerpMetricParamsByPositionForAPI
        :rtype:
        """

        index = pos_metric_params.index
        res = pos_metric_params.result

        rel_val = self.rel_scale(res.get_scale("RELEVANCE"))

        tw_str = res.get_scale("SEARCH_TW_FACTOR")
        if tw_str is None:
            tw_val = 0.5
        else:
            tw_val = float(tw_str)

        click_str = res.get_scale("SEARCH_CLICKS_FACTOR")
        if click_str is None:
            click = 0.5
        else:
            click = float(click_str)

        pos = index + 1

        if res.has_scale("DUPLICATE_FULL"):
            dups_before = res.get_scale("DUPLICATE_FULL")[:index].count("1")
        else:
            dups_before = 0
        dup_weight = 1.0 / (1.0 + dups_before)
        discounted = (self.rel_weight * rel_val + self.tw_weight * tw_val) / float(pos) * dup_weight
        not_discounted = self.click_weight * click
        val_by_pos = discounted + not_discounted
        return val_by_pos

    @staticmethod
    def aggregate_by_position(agg_metric_params):
        """
        :type agg_metric_params: SerpMetricAggregationParamsForAPI
        :rtype: float
        """
        return sum([x.value for x in agg_metric_params.pos_metric_values])
