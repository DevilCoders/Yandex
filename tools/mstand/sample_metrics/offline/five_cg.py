import math

from metrics_api.offline import SerpMetricParamsForAPI  # noqa


def sigmoid(x):
    return 1.0 / (1.0 + math.exp(-x))


class FiveCG:
    def __init__(self, depth=30, rel_weight=0.45, tw_weight=0.1, click_weight=0.45):
        self.depth = depth
        self.rel_weight = rel_weight
        self.tw_weight = tw_weight
        self.click_weight = click_weight

    def precompute_serpset(self, metric_params):
        pass

    # def precompute(self, metric_params):
    #     pass

    def scale_stat_depth(self):
        return self.depth

    @staticmethod
    def rel_scale(rel_mark):
        rel_map = {
            'VITAL': 1.0,
            'USEFUL': 0.75,
            'RELEVANT_PLUS': 0.5,
            'RELEVANT_MINUS': 0.25,
            'IRRELEVANT': 0.0,
            'SOFT_404': 0.0,
            '_404': 0.0,
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

        # # MSTAND-933 testing
        # if len(metric_params.query_text) > 75:
        #     raise Exception("Error in FiveCG")

        values_by_pos = []
        for index, res in enumerate(results):

            # if res.is_turbo:
            #     logging.info("originalUrl: %s", res.original_url)
            #
            # if res.is_wizard:
            #     logging.info("this is a wizard, url = %s", res.url)

            # if res.is_amp:
            #     logging.info("isAmp")

            rel_val = self.rel_scale(res.get_scale("RELEVANCE"))

            # if res.site_links:
            #     rel_val += 100500

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

            if res.has_scale("DUPLICATE_FULL"):
                dups_before = res.get_scale("DUPLICATE_FULL")[:index].count("1")
            else:
                dups_before = 0
            dup_weight = 1.0 / (1.0 + dups_before)
            discounted = (self.rel_weight * rel_val + self.tw_weight * tw_val) / float(pos) * dup_weight
            not_discounted = self.click_weight * click
            pos_value = discounted + not_discounted

            values_by_pos.append(pos_value)

        return sum(values_by_pos)
