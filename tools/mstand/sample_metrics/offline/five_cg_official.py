import math

from metrics_api.offline import SerpMetricParamsForAPI  # noqa


def sigmoid(x):
    return 1.0 / (1.0 + math.exp(-x))


class FiveCGOfficial:
    def __init__(self, depth, rel_weight=0.5, tw_weight=0.05, click_weight=0.45):
        assert depth is not None
        assert rel_weight is not None
        assert tw_weight is not None
        assert click_weight is not None

        assert round(math.fabs(rel_weight + tw_weight + click_weight - 1.0), 5) < 0.0001
        self.depth = depth
        self.rel_weight = rel_weight
        self.tw_weight = tw_weight
        self.click_weight = click_weight

    @staticmethod
    def rel_scale(rel_mark):
        rel_map = {
            'VITAL': 1.0,
            'USEFUL': 0.75,
            'RELEVANT_PLUS': 0.5,
            'RELEVANT_MINUS': 0.25,
            'IRRELEVANT': 0.0,
            '_404': 0.0,
            'SOFT_404': 0.0,
            'VIRUS': 0.0,
            'NOT_JUDGED': 0.0,
            'AUX': 0.0,
            None: 0.0,
        }
        return rel_map.get(rel_mark, 0.0)

    # metric value for one serp
    def value(self, metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: float
        """
        results = metric_params.results[:self.depth]

        values_by_pos = []
        for index, res in enumerate(results):

            rel_val = self.rel_scale(res.get_scale("RELEVANCE"))

            tw_map = {
                "HIGHEST": 1.0,
                "HIGH": 0.75,
                "MIDDLE": 0.5,
                "LOW": 0.25,
                "LOWEST": 0.0,
                "_404": 0.0,
                "VIRUS": 0.0,
                None: None,
            }
            tw_val = tw_map.get(res.get_scale("TW_HOST"))
            if tw_val is None:
                tw_str = res.get_scale("SEARCH_TW_FACTOR")
                if tw_str is None:
                    tw_val = 0.5
                else:
                    tw_val = float(tw_str)

            click_str = res.get_scale("FIVE_CG_CLICKS_FACTOR")
            if click_str is None:
                click_value = 0
            else:
                click_value = float(click_str)
            click = sigmoid(click_value)

            pos = index + 1

            dup_weight = 1

            if res.has_scale("DUPLICATE_FULL_TOLOKA"):
                full_dups_before = res.get_scale("DUPLICATE_FULL_TOLOKA")[:index].count("1")
                if full_dups_before > 0:
                    dup_weight = 0

            if res.has_scale("DUPLICATE_FULL_NEVASCA"):
                dups_before = res.get_scale("DUPLICATE_FULL_NEVASCA")[:index].count("1")
                if dups_before > 0:
                    dup_weight = min(dup_weight, 0.85 ** (index * dups_before))

            assert dup_weight is not None
            assert rel_val is not None
            assert tw_val is not None

            rel_plus_tw = self.rel_weight * rel_val + self.tw_weight * tw_val
            discounted = dup_weight * rel_plus_tw / float(pos)
            not_discounted = self.click_weight * click
            pos_value = discounted + not_discounted

            values_by_pos.append(pos_value)

        result = sum(values_by_pos)

        return result
