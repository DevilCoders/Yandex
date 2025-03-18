import logging

from collections import OrderedDict
from user_plugins import PluginKey
import yaqutils.misc_helpers as umisc


@umisc.hash_and_ordering_from_key_method
class CorrelationResult(object):
    def __init__(self,
                 left_metric,
                 right_metric,
                 corr_value=None,
                 left_sensitivity=None,
                 right_sensitivity=None,
                 additional_info=""):
        """
        :type left_metric: PluginKey
        :type right_metric: PluginKey
        :type corr_value: float | (float, float) | None
        :type left_sensitivity: float | None
        :type right_sensitivity: float | None
        :type additional_info: str
        """
        self.left_metric = left_metric
        self.right_metric = right_metric
        self.corr_value = corr_value
        self.left_sensitivity = left_sensitivity
        self.right_sensitivity = right_sensitivity
        self.additional_info = additional_info

    def serialize(self):
        result = OrderedDict()
        result["left_metric"] = self.left_metric.serialize()
        result["right_metric"] = self.right_metric.serialize()
        result["corr_value"] = self._serialize_corr_value()
        result["left_sensitivity"] = self.left_sensitivity
        result["right_sensitivity"] = self.right_sensitivity
        result["additional_info"] = self.additional_info
        return result

    def _serialize_corr_value(self):
        # typically, correlation function returns tuple of 2 values
        value = self.corr_value
        if isinstance(value, tuple):
            return tuple([umisc.serialize_float(x) for x in value])
        else:
            return umisc.serialize_float(value)

    @staticmethod
    def deserialize(corr_result_data):
        """
        :type corr_result_data: dict
        :rtype: CorrelationResult
        """
        left_metric_data = corr_result_data["left_metric"]
        right_metric_data = corr_result_data["right_metric"]

        left_metric = PluginKey.deserialize(left_metric_data)
        right_metric = PluginKey.deserialize(right_metric_data)
        corr_value = corr_result_data["corr_value"]
        left_sensitivity = corr_result_data["left_sensitivity"]
        right_sensitivity = corr_result_data["right_sensitivity"]
        additional_info = corr_result_data["additional_info"]

        return CorrelationResult(left_metric,
                                 right_metric,
                                 corr_value,
                                 left_sensitivity,
                                 right_sensitivity,
                                 additional_info)

    def key(self):
        return (
            self.left_metric.key(),
            self.right_metric.key(),
            self.corr_value,
            self.left_sensitivity,
            self.right_sensitivity,
            self.additional_info,
        )

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "CorrRes({} vs {}: {}, {} vs {}, {!r})".format(self.left_metric,
                                                              self.right_metric,
                                                              self.corr_value,
                                                              self.left_sensitivity,
                                                              self.right_sensitivity,
                                                              self.additional_info)

    @staticmethod
    def tsv_header():
        header = (
            "left_metric",
            "right_metric",
            "corr_value",
            "left_sensitivity",
            "right_sensitivity",
            "additional_info",
        )
        return "\t".join(header)

    def tsv_line(self):
        row = (
            self.left_metric.str_key(),
            self.right_metric.str_key(),
            self.corr_value,
            self.left_sensitivity,
            self.right_sensitivity,
            clear_additional_info_symbols(self.additional_info),
        )
        return "\t".join(str(x) for x in row)

    @staticmethod
    def sorted_by_value(results):
        return sorted(results, key=lambda r: r.corr_value, reverse=True)


def clear_additional_info_symbols(additional_info):
    for whitespace in "\t\r\n":
        if whitespace in additional_info:
            logging.warning("Will replace 0x%02X symbols with spaces in additional info", ord(whitespace))
            additional_info = additional_info.replace(whitespace, " ")
    return additional_info


class CriteriaPair(object):
    def __init__(self, criteria_left, criteria_right):
        """
        :type criteria_left: PluginKey | None
        :type criteria_right: PluginKey | None
        """
        self.criteria_left = criteria_left
        self.criteria_right = criteria_right

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "CrPair({}:{})".format(self.criteria_left, self.criteria_right)


class CorrelationParams(object):
    def __init__(self, exp_pair, metric_pair, criteria_pair):
        """
        :type exp_pair: ExperimentPair
        :type metric_pair: CorrelationResult
        :type criteria_pair: CriteriaPair
        """
        self.exp_pair = exp_pair
        self.metric_pair = metric_pair
        self.criteria_pair = criteria_pair

    def serialize(self):
        return {
            "exp_pair": self.exp_pair.serialize(),
            "metric_pair": self.metric_pair.serialize(),
        }
