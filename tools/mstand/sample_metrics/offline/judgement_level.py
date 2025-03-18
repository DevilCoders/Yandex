# (c) olycha
import logging
import yaqutils.six_helpers as usix
from experiment_pool import MetricColoring

from metrics_api.offline import SerpMetricParamsByPositionForAPI  # noqa
from metrics_api.offline import SerpMetricAggregationParamsForAPI  # noqa


# valid schemes:
# "scale"
# ["scale1", "scale2", "scale3"]
# {"scale": ["nested_scale1", "nested_scale2"]}
# ["scale1", "scale2", {"scale3": ["nested_scale1", "nested_scale2"]}]

def result_has_scales(result, scale_scheme):
    """
    :type result:
    :type scale_scheme: str | list[str] | dict[str]
    :return:
    """
    if isinstance(scale_scheme, str):
        if hasattr(result, "has_scale"):
            return result.has_scale(scale_scheme)
        elif isinstance(result, dict):
            return scale_scheme in result
        else:
            raise Exception("Unsupported type for 'result' object: {} [str case]", type(result))
    elif isinstance(scale_scheme, list):
        return all(result_has_scales(result, one_scale) for one_scale in scale_scheme)
    elif isinstance(scale_scheme, dict):
        for root_scale, nested_scale_scheme in usix.iteritems(scale_scheme):
            if not result_has_scales(result, root_scale):
                return False
            if hasattr(result, "scales"):
                nested_result = result.scales[root_scale]
            elif isinstance(result, dict):
                nested_result = result[root_scale]
            else:
                raise Exception("Unsupported type for 'result' object [dict case]: {}", type(result))
            if not result_has_scales(nested_result, nested_scale_scheme):
                return False
        return True
    else:
        raise Exception("Unexpected type in scale_scheme: {}", type(scale_scheme))


# Universal judgement level metric
class JudgementLevel(object):
    coloring = MetricColoring.MORE_IS_BETTER

    def __init__(self, scales, depth=5):
        if not isinstance(scales, (str, list, dict)):
            raise Exception("Wrong type for required scale scheme. Expected str, list or dict.")
        self.required_scales = scales

        self.depth = depth

        logging.info("JudgementLevel metric: scales = %s, depth = %s", self.required_scales, self.depth)

    def scale_stat_depth(self):
        return self.depth

    def value_by_position(self, pos_metric_values):
        """
        :type pos_metric_values: SerpMetricParamsByPositionForAPI
        :rtype: float
        """

        if result_has_scales(pos_metric_values.result, self.required_scales):
            return 1
        else:
            return 0

    def aggregate_by_position(self, agg_metric_values):
        """
        :type agg_metric_values: SerpMetricAggregationParamsForAPI
        :return:
        """
        if self.depth is not None:
            depth_slice = agg_metric_values.pos_metric_values[:self.depth]
        else:
            depth_slice = agg_metric_values.pos_metric_values

        if not depth_slice:
            return 1.0
        else:
            res = float(sum(mv.value for mv in depth_slice)) / len(depth_slice)
            # if res < 1.0:
            #     logging.info("qid = %s, res = %1.4f", agg_metric_values.qid, res)
            return res
