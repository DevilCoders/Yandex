import logging

from metrics_api.offline import SerpMetricParamsByPositionForAPI  # noqa
from metrics_api.offline import SerpMetricAggregationParamsForAPI  # noqa

from metrics_api.offline import SerpMetricParamsForAPI  # noqa
from metrics_api.offline import ResultTypeEnumForAPI
from metrics_api.offline import AlignmentEnumForAPI

from experiment_pool import MetricColoring


# here are metrics used in unit tests (computation code consistency check)


def discount(index):
    return 10 ** index


# sum of all results relevances for each result
class OfflineTestMetricRelSum:
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self):
        pass

    @staticmethod
    def rel_scale(rel_mark):
        rel_map = {
            "VITAL": 0.61,
            "USEFUL": 0.41,
            "RELEVANT_PLUS": 0.14,
            "RELEVANT_MINUS": 0.07,
            "IRRELEVANT": 0.0,
            "_404": 0.0,
            "SOFT_404": 0.0,
            "VIRUS": 0.0,
            "NOT_JUDGED": 0.0,
            "AUX": 0.0,
            None: 0.0,
        }

        return rel_map.get(rel_mark, 0.0)

    @staticmethod
    def value(metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: float
        """
        res_values = []
        for index, res in enumerate(metric_params.results):
            relevance = OfflineTestMetricRelSum.rel_scale(res.get_scale("RELEVANCE"))
            logging.info("relevance = %s", relevance)
            # to remove difference of 'round' behavior in python2/3
            val = float(round(relevance * 100) * discount(index))
            res_values.append(val)
        return sum(res_values)


# mean URL length across all results
class OfflineTestMetricSumUrlLen:
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self):
        pass

    @staticmethod
    def value(metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: float
        """

        url_lengths = []
        for index, res in enumerate(metric_params.results):
            val = len(res.url) * discount(index)
            url_lengths.append(val)

        return sum(url_lengths)


class OfflineTestMetricYield:
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self):
        pass

    @staticmethod
    def value(metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: float
        """
        if not metric_params.results:
            yield 0.0

        for index, res in enumerate(metric_params.results):
            yield len(res.url) * discount(index)


class OfflineTestMetricNothing:
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self):
        pass

    @staticmethod
    def value(metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: None
        """
        # test case when metric returns None in some cases
        if metric_params.qid == 1:
            return 12.34
        else:
            return None


class OfflineTestMetricNotNumbers:
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self):
        pass

    # metric value for one serp
    @staticmethod
    def value(metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: None
        """
        # test case when metric returns None
        return str(len(metric_params.results))


class OfflineTestMetricBuggy:
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self):
        pass

    # metric value for one serp
    @staticmethod
    def value(metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: None
        """
        # test case when metric throws an exception
        if metric_params.qid == 2:
            raise Exception("Buggy metric raised an Exception")
        else:
            return metric_params.qid


class OfflineTestMetricParamsFields:
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self):
        pass

    # metric value for one serp
    @staticmethod
    def value(metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: None
        """

        # This metric has no real sense.
        # It just tests all most of API fields.
        yield len(metric_params.experiment.serpset_id)
        yield metric_params.qid
        yield len(metric_params.query_text)
        yield metric_params.query_region
        yield metric_params.query_device
        yield len(metric_params.query_uid)
        yield len(metric_params.query_country) if metric_params.query_country else 123
        yield len(metric_params.query_map_info) if metric_params.query_map_info else 456

        yield len(metric_params.serp_data)

        logging.info("result count: %d", len(metric_params.results))

        for pos_markup in metric_params.results:
            yield pos_markup.pos
            yield len(pos_markup.url)
            yield pos_markup.is_wizard
            yield pos_markup.wizard_type
            yield pos_markup.is_fast_robot_src
            yield pos_markup.alignment
            yield pos_markup.result_type
            yield pos_markup.json_slices
            yield len(pos_markup.scales)


class OfflineTestMetricDetailedIncomplete:
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self):
        pass

    # broken detailed metric
    @staticmethod
    def value_by_position(_):
        pass


class OfflineTestMetricWithError:
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self):
        pass

    @staticmethod
    def value(_):
        raise Exception("Exception from test metric with error inside value() method")


class OfflineTestMetricAlwaysNull:
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self):
        pass

    @staticmethod
    def value(_):
        return None


class OfflineTestMetricDetailed:
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self):
        pass

    @staticmethod
    def rel_scale(rel_mark):
        rel_map = {
            'VITAL': 1.0,
            'USEFUL': 0.9,
            'RELEVANT_PLUS': 0.8,
            'RELEVANT_MINUS': 0.7,
            'IRRELEVANT': 0.05,
            '404': 0.04,
            'VIRUS': 0.03,
            'NOT_JUDGED': 0.02,
            None: 0.01,
        }
        return rel_map[rel_mark]

    # per-position aggregation function
    @staticmethod
    def aggregate_by_position(agg_metric_params):
        """
        :type agg_metric_params: SerpMetricAggregationParamsForAPI
        :rtype: float
        """
        assert agg_metric_params.qid is not None
        agg_metric_params.get_serp_data("NON_EXISTING_SERP_DATA")
        assert "NON_EXISTING_SERP_DATA" in agg_metric_params.serp_data_stats.missing
        assert "NON_EXISTING_SERP_DATA" in agg_metric_params.serp_data_stats.used
        return sum([x.value * discount(x.index) for x in agg_metric_params.pos_metric_values])

    # metric value for one serp component
    @staticmethod
    def value_by_position(pos_params):
        """
        :type pos_params: SerpMetricParamsByPositionForAPI
        :rtype: float
        """
        pos_params.result.get_scale("NON_EXISTING_SCALE")
        return OfflineTestMetricDetailed.rel_scale(pos_params.result.get_scale("RELEVANCE")) * (0.95 ** pos_params.index)


class OfflineTestMetricDetailedWithPrecompute:
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self):
        self.precomputed_value = 0
        pass

    def precompute(self, metric_params):
        self.precomputed_value = 1

    # metric value for one serp component
    def value_by_position(self, pos_params):
        """
        :type pos_params: SerpMetricParamsByPositionForAPI
        :rtype: float
        """
        assert self.precomputed_value == 1

    def aggregate_by_position(self, agg_metric_params):
        """
        :type agg_metric_params: SerpMetricAggregationParamsForAPI
        :return:
        """
        return agg_metric_params.qid


class OfflineTestMetricDetailedWithDepth:
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self):
        self.depth = 2

    def serp_depth(self):
        return self.depth

    # metric value for one serp component
    def value_by_position(self, pos_params):
        """
        :type pos_params: SerpMetricParamsByPositionForAPI
        :rtype: float
        """
        return 10 ** pos_params.pos

    def aggregate_by_position(self, agg_metric_params):
        """
        :type agg_metric_params: SerpMetricAggregationParamsForAPI
        :return:
        """
        assert len(agg_metric_params.pos_metric_values) <= self.depth
        serp_sum = sum([x.value for x in agg_metric_params.pos_metric_values])
        # factor 30 used to get integer numbers in average
        return 30 * serp_sum


class OfflineTestMetricProductionPFound:
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self):
        pass

    # metric value for one serp
    @staticmethod
    def value(metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: None
        """
        return metric_params.get_serp_data("metric.onlySearchResult.pfound", 0.0)


class OfflineTestMetricCountry:
    coloring = MetricColoring.NONE

    def __init__(self):
        pass

    # metric value for one serp
    @staticmethod
    def value(metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: float
        """
        if metric_params.query_country:
            return 1
        else:
            return 0


class OfflineTestGeoMetric:
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self):
        pass

    @staticmethod
    def value(metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: float
        """

        # WARNING: this "metric" has NO real sense, it's just an example.
        map_info = metric_params.query_map_info
        if map_info:
            query_x, query_y = map_info["x"], map_info["y"]
        else:
            query_x, query_y = None, None

        # --serp-attrs coordinates.mapBottomLeft,coordinates.mapTopRight
        one = metric_params.serp_data.get("coordinates.mapBottomLeft")
        two = metric_params.serp_data.get("coordinates.mapTopRight")
        logging.debug("coords: %s %s", one, two)

        total_distance = 0.0
        for index, res in enumerate(metric_params.results):
            if res.coordinates:
                res_x, res_y = res.coordinates["longitude"], res.coordinates["latitude"]
            else:
                res_x, res_y = None, None

            if query_x and query_y and res_x and res_y:
                distance = (query_x - res_x) ** 2 + (query_y - res_y) ** 2
                total_distance += distance * (1.0 / (1.0 + index))
        return total_distance


class OfflineTestEnumsMetric:
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self):
        pass

    @staticmethod
    def value(metric_params):
        """
        :type metric_params: SerpMetricParamsForAPI
        :rtype: float
        """
        result = 0
        for index, res in enumerate(metric_params.results):
            if res.result_type == ResultTypeEnumForAPI.SEARCH_RESULT:
                result += 1
            if res.alignment == AlignmentEnumForAPI.LEFT:
                result += 2
        return result


class OfflineTestNanInfMetric:
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self):
        pass

    @staticmethod
    def value(_):
        """
        :rtype: float
        """
        return float('nan')


class OfflineTestBoolMetric:
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self):
        pass

    @staticmethod
    def value(_):
        """
        :rtype: bool
        """
        return True
