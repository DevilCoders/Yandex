import logging
from metrics_api.offline import SerpMetricParamsForAPI  # noqa


class VideoPFound(object):
    coloring = "more-is-better"

    def __init__(self, depth=30):
        self.depth = depth

    def value(self, metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: float
        """

        rel_map = {
            "REL": 0.15,
            "NOT_REL": 0.0,
            "404": 0.0,
            "": 0.0,
            None: 0.0,
        }

        rels_by_pos = []
        for result in metric_params.results[:self.depth]:
            rel_str = result.get_scale("video_light_relevance")
            rel_value = rel_map[rel_str]

            if result.get_scale("VDP") == "BAD":
                rel_value = 0.0

            rels_by_pos.append(rel_value)

        logging.debug("qid %s, rels: %s", metric_params.qid, rels_by_pos)

        prob_break = 0.15
        look_probs = {0: 1.0}
        for index, rel_value in enumerate(rels_by_pos):
            prob_skip = 1.0 - rel_value
            prob_continue = 1.0 - prob_break
            look_probs[index + 1] = look_probs[index] * prob_skip * prob_continue

        pfound = sum([look_probs[index] * rel for index, rel in enumerate(rels_by_pos)])
        return pfound
