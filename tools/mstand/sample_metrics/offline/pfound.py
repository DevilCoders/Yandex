from metrics_api.offline import SerpMetricParamsForAPI  # noqa


class Pfound:
    def __init__(self, depth=5):
        self.depth = depth

    @staticmethod
    def rel_scale(rel_mark):
        rel_map = {
            'VITAL': 0.61,
            'USEFUL': 0.41,
            'RELEVANT_PLUS': 0.14,
            'RELEVANT_MINUS': 0.07,
            'IRRELEVANT': 0.0,
            '_404': 0.0,
            'SOFT_404': 0.0,
            'VIRUS': 0.0,
            'NOT_JUDGED': 0.0,
            None: 0.0,
        }
        return rel_map[rel_mark]

    # metric value for one serp
    def value(self, metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: float
        """

        rels_by_pos = []
        for index, result in enumerate(metric_params.results[:self.depth]):
            rel_value = self.rel_scale(result.get_scale("RELEVANCE"))
            rels_by_pos.append(rel_value)

        look_probs = {0: 1.0}
        for index, rel_value in enumerate(rels_by_pos):
            look_probs[index + 1] = look_probs[index] * (1 - rel_value) * 0.85

        # logging.info("qid %s, lp: %s", metric_params.qid, look_probs)

        pfound = sum([look_probs[index] * rel for (index, rel) in enumerate(rels_by_pos)])
        return pfound
