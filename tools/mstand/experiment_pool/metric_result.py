import logging
from collections import OrderedDict

import yaqutils.misc_helpers as umisc
import yaqutils.math_helpers as umath
import yaqutils.six_helpers as usix
from experiment_pool import CriteriaResult
from experiment_pool import PoolException
from user_plugins import PluginKey
from user_plugins import PluginABInfo
from mstand_structs import LampKey
from mstand_structs import SqueezeVersions


class MetricStats(object):
    def __init__(self, values):
        """
        :type values: list[float]
        """
        self.max_val = umath.max_of_nullable(values)
        self.min_val = umath.min_of_nullable(values)

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "metric value stats: min: {}, max: {}".format(self.min_val, self.max_val)

    def serialize(self):
        serialized = OrderedDict()
        serialized["min_val"] = umisc.serialize_float(self.min_val)
        serialized["max_val"] = umisc.serialize_float(self.max_val)
        return serialized

    def all_values(self):
        return [self.max_val, self.min_val]

    @staticmethod
    def aggregate_stats(detailed_stats):
        """
        :type detailed_stats: dict[PluginKey, list[float]]
        :rtype: dict[PluginKey, MetricStats]
        """
        agg_stats = {}
        for metric_key, values in usix.iteritems(detailed_stats):
            agg_stats[metric_key] = MetricStats(values)
        return agg_stats


# noinspection PyClassHasNoInit
class MetricColoring:
    NONE = "none"
    MORE_IS_BETTER = "more-is-better"
    LESS_IS_BETTER = "less-is-better"
    LAMP = "lamp"
    ALL = {NONE, MORE_IS_BETTER, LESS_IS_BETTER, LAMP}

    @staticmethod
    def set_metric_instance_coloring(metric_instance, forced_coloring):
        """
        :type metric_instance:
        :type forced_coloring: str
        :rtype: None
        """
        if forced_coloring is not None:
            if forced_coloring not in MetricColoring.ALL:
                raise PoolException("Invalid metric coloring: {}".format(forced_coloring))
            logging.debug("Setting forced coloring: %s", forced_coloring)

            metric_instance.coloring = forced_coloring
        elif hasattr(metric_instance, "coloring"):
            logging.debug("Using native metric coloring: %s", metric_instance.coloring)
        else:
            logging.debug("Using default coloring: %s", MetricColoring.MORE_IS_BETTER)
            metric_instance.coloring = MetricColoring.MORE_IS_BETTER


# noinspection PyClassHasNoInit
class MetricColor:
    GREEN = "green"
    RED = "red"
    YELLOW = "yellow"
    GRAY = "gray"
    ALL = {GREEN, RED, YELLOW, GRAY}


# noinspection PyClassHasNoInit
class MetricDataType:
    NONE = "none"
    VALUES = "values"
    KEY_VALUES = "key-values"
    ALL = {NONE, VALUES, KEY_VALUES}


# noinspection PyClassHasNoInit
class MetricType:
    ONLINE = "online"
    OFFLINE = "offline"
    ALL = {ONLINE, OFFLINE}


# noinspection PyClassHasNoInit
class MetricValueType:
    AVERAGE = "average"
    SUM = "sum"
    ALL = {AVERAGE, SUM}

    @staticmethod
    def set_metric_instance_value_type(metric_instance):
        value_type = getattr(metric_instance, "value_type", MetricValueType.AVERAGE)
        if value_type not in MetricValueType.ALL:
            raise Exception("Unknown metric value type: {}".format(value_type))
        metric_instance.value_type = value_type


@umisc.hash_and_ordering_from_key_method
class MetricValues(object):
    def __init__(self,
                 count_val,
                 sum_val,
                 average_val,
                 data_type,
                 data_file=None,
                 row_count=None,
                 value_type=MetricValueType.AVERAGE):
        """
        :type count_val: int None
        :type sum_val: float None
        :type average_val: float None
        :type data_type: str
        :type data_file: str None
        :type row_count: int None
        :type value_type: str
        """
        self.count_val = count_val  # numeric values count
        self.sum_val = sum_val
        self.average_val = average_val
        self.row_count = row_count  # total row count

        if data_type not in MetricDataType.ALL:
            raise PoolException("Invalid data_type field in MetricValues: {}".format(data_type))
        self.data_type = data_type

        self.data_file = data_file

        if value_type not in MetricValueType.ALL:
            raise PoolException("Invalid value_type field in MetricValues: {}".format(value_type))
        self.value_type = value_type

    def serialize(self):
        result = OrderedDict()
        result["count"] = self.count_val
        result["average"] = self.average_val
        result["sum"] = self.sum_val
        result["value_type"] = self.value_type
        result["data_type"] = self.data_type
        if self.data_file:
            result["data_file"] = self.data_file
        if self.row_count is not None:
            result["row_count"] = self.row_count
        return result

    @staticmethod
    def deserialize(metric_values_data):
        """
        :type metric_values_data: dict
        :rtype: MetricValues
        """

        count_val = metric_values_data.get("count")
        if count_val is not None and not umisc.is_integer(count_val):
            raise PoolException("Incorrect 'count' field in metric result (expected integer): {}".format(count_val))

        average_val = metric_values_data.get("average")
        if average_val is not None and not umisc.is_number(average_val):
            raise PoolException("Incorrect 'average' field in metric result (expected number): {}".format(average_val))

        sum_val = metric_values_data.get("sum")
        if sum_val is not None and not umisc.is_number(sum_val):
            raise PoolException("Incorrect 'sum' field in metric result (expected number): {}".format(sum_val))

        row_count = metric_values_data.get("row_count")
        if row_count is not None and not umisc.is_number(row_count):
            raise PoolException("Incorrect 'row_count' field in metric result (expected number): {}".format(row_count))

        data_file = metric_values_data.get("data_file")

        data_type = metric_values_data.get("data_type", MetricDataType.VALUES)
        if data_type not in MetricDataType.ALL:
            raise PoolException("Incorrect data_type in metric result: {}".format(data_type))

        value_type = metric_values_data.get("value_type", MetricValueType.AVERAGE)
        if value_type not in MetricValueType.ALL:
            raise PoolException("Incorrect value_type in metric result: {}".format(value_type))

        return MetricValues(count_val=count_val,
                            sum_val=sum_val,
                            average_val=average_val,
                            data_type=data_type,
                            data_file=data_file,
                            value_type=value_type,
                            row_count=row_count)

    def __str__(self):
        return "MetVal(count={}, avg={}, sum={}, vt={}, dt={}, f={}, rows={})".format(
            self.count_val,
            self.average_val,
            self.sum_val,
            self.value_type,
            self.data_type,
            self.data_file,
            self.row_count
        )

    def __repr__(self):
        return str(self)

    def key(self):
        return self.row_count, self.count_val, self.sum_val, self.average_val, self.data_type

    @property
    def significant_value(self):
        if self.value_type == MetricValueType.AVERAGE:
            return self.average_val
        else:
            return self.sum_val


@umisc.hash_and_ordering_from_key_method
class MetricResult(object):
    def __init__(
            self,
            metric_key,
            metric_type,
            metric_values,
            coloring=MetricColoring.NONE,
            invalid_days=0,
            criteria_results=None,
            extra_data=None,
            version=None,
            result_table_path=None,
            result_table_field=None,
            ab_info=None,
    ):
        """
        :type metric_key: PluginKey
        :type metric_type: str
        :type metric_values: MetricValues
        :type coloring: str
        :type invalid_days: int
        :type criteria_results: list[CriteriaResult]
        :type extra_data: object
        :type version: SqueezeVersions | dict
        :type result_table_path: str | None
        :type result_table_field: str | None
        :type ab_info: PluginABInfo | None
        """
        if not isinstance(metric_key, PluginKey):
            raise PoolException("Incorrect type passed as metric key - expected PluginKey")

        if not isinstance(metric_values, MetricValues):
            raise PoolException("Incorrect type passed as metric values - expected MetricValues")

        if metric_type not in MetricType.ALL:
            raise PoolException("Incorrect metric type: {}".format(metric_type))
        self.metric_type = metric_type

        self.metric_key = metric_key
        self.metric_values = metric_values
        self.coloring = coloring
        if coloring not in MetricColoring.ALL:
            raise PoolException("Incorrect metric coloring: {}".format(coloring))
        self.invalid_days = invalid_days
        self.extra_data = extra_data

        if criteria_results is None:
            criteria_results = []
        self.criteria_results = criteria_results

        if version is None:
            version = {}
        self.version = version

        self.result_table_path = result_table_path
        self.result_table_field = result_table_field

        if ab_info is not None and not isinstance(ab_info, PluginABInfo):
            raise PoolException("Incorrect type passed as ab info - expected ABInfo")
        self.ab_info = ab_info

    def get_criteria_results_map(self):
        return {crit_res.key(): crit_res for crit_res in self.criteria_results}

    def find_criteria_result(self, criteria_key):
        cr_map = self.get_criteria_results_map()
        return cr_map.get(criteria_key)

    def metric_name(self):
        return self.metric_key.pretty_name()

    def metric_group(self):
        if self.ab_info is not None:
            return self.ab_info.group
        else:
            return None

    def serialize(self):
        result = OrderedDict()

        metric_key = self.metric_key.serialize()

        result["metric_key"] = metric_key

        metric_values = self.metric_values.serialize()

        result["values"] = metric_values

        if self.coloring != MetricColoring.NONE:
            result["coloring"] = self.coloring

        result["type"] = self.metric_type

        if self.invalid_days is not None and self.invalid_days > 0:
            result["invalid_days"] = self.invalid_days

        if self.extra_data is not None:
            result["extra_data"] = self.extra_data

        if self.criteria_results:
            synt_criterias = [cr for cr in self.criteria_results if cr.synthetic]
            if synt_criterias:
                result["synthetic_criterias"] = umisc.serialize_array(synt_criterias)

            reg_criterias = [cr for cr in self.criteria_results if not cr.synthetic]
            if reg_criterias:
                result["criterias"] = umisc.serialize_array(reg_criterias)

        if self.version:
            result["version"] = self.version.serialize()

        if self.result_table_path:
            result["result_table_path"] = self.result_table_path
        if self.result_table_field:
            result["result_table_field"] = self.result_table_field

        if self.ab_info is not None and not self.ab_info.empty():
            result["ab_info"] = self.ab_info.serialize()

        return result

    @staticmethod
    def deserialize(metric_result_data):
        """
        :type metric_result_data: dict
        :rtype: MetricResult
        """
        name = metric_result_data.get("name")
        if name:
            metric_key = PluginKey(name=name)
        else:
            metric_key = PluginKey.deserialize(metric_result_data["metric_key"])

        values_data = metric_result_data.get("values")
        if values_data:
            metric_values = MetricValues.deserialize(values_data)
        else:
            metric_values = MetricValues(
                count_val=None,
                sum_val=metric_result_data.get("sum"),
                average_val=metric_result_data.get("average"),
                data_type=metric_result_data["data_type"],
                data_file=metric_result_data.get("data_file"),
            )

        coloring = metric_result_data.get("coloring", MetricColoring.NONE)
        if coloring not in MetricColoring.ALL:
            raise PoolException("Incorrect coloring in metric result: {}".format(coloring))

        metric_type = metric_result_data.get("type", MetricType.ONLINE)

        invalid_days = metric_result_data.get("invalid_days", 0)
        if not isinstance(invalid_days, int):
            raise PoolException("Incorrect invalid_days type in metric result (expected int): {}".format(invalid_days))

        extra_data = metric_result_data.get("extra_data")

        result_table_path = metric_result_data.get("result_table_path")
        result_table_field = metric_result_data.get("result_table_field")

        ab_info = PluginABInfo.deserialize(metric_result_data.get("ab_info"))

        metric_result = MetricResult(
            metric_key=metric_key,
            metric_values=metric_values,
            metric_type=metric_type,
            coloring=coloring,
            invalid_days=invalid_days,
            extra_data=extra_data,
            result_table_path=result_table_path,
            result_table_field=result_table_field,
            ab_info=ab_info,
        )

        criterias_data = metric_result_data.get("criterias", [])
        metric_result.criteria_results = umisc.deserialize_array(criterias_data, CriteriaResult)

        version = metric_result_data.get("version", {})
        if version:
            metric_result.version = SqueezeVersions.deserialize(version)

        return metric_result

    def __str__(self):
        if self.invalid_days is not None and self.invalid_days > 0:
            inv_days = ", inv.days={}".format(self.invalid_days)
        else:
            inv_days = ""
        if self.extra_data:
            extra = ", extra={}".format(self.extra_data)
        else:
            extra = ""

        if self.version:
            version = ", version={}".format(self.version)
        else:
            version = ""

        return "MetRes({}, {}, {}, {}, {}, {})".format(self.metric_key,
                                                       self.metric_values,
                                                       self.criteria_results,
                                                       inv_days, extra, version)

    def __repr__(self):
        return str(self)

    def key(self):
        return self.metric_key

    def add_criteria_result(self, criteria_result):
        """
        :type criteria_result: CriteriaResult
        :rtype:
        """
        if criteria_result.criteria_key in self.all_criteria_keys():
            raise PoolException("Duplicate criteria result: {} for metric res {}".format(criteria_result, self))
        self.criteria_results.append(criteria_result)

    def all_criteria_keys(self):
        """
        :rtype: set[PluginKey]
        """
        criteria_keys = set()
        for criteria_result in self.criteria_results:
            criteria_keys.add(criteria_result.criteria_key)
        return criteria_keys

    def sort(self):
        self.criteria_results.sort(key=lambda x: x.key())


class MetricDiff(object):
    def __init__(self, control_values, exp_values):
        """
        :type control_values: MetricValues
        :type exp_values: MetricValues
        """
        self.average = MetricValueDiff(control_values.average_val, exp_values.average_val)
        self.sum = MetricValueDiff(control_values.sum_val, exp_values.sum_val)
        self.count = MetricValueDiff(control_values.count_val, exp_values.count_val)
        self.significant = MetricValueDiff(control_values.significant_value, exp_values.significant_value)

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "MetricDiff(average={}, sum={})".format(self.average, self.sum)


class MetricValueDiff(object):
    def __init__(self, control_value, exp_value):
        """
        :type control_value: float
        :type exp_value: float
        """
        self.abs_diff = None
        self.rel_diff = None
        self.rel_diff_human = None
        self.perc_diff = None
        self.perc_diff_human = None

        if control_value is None or exp_value is None:
            return

        self.abs_diff = exp_value - control_value

        control_value_abs = abs(control_value)
        if control_value_abs < 1e-14:
            return

        self.rel_diff = float(self.abs_diff) / control_value
        self.rel_diff_human = float(self.abs_diff) / control_value_abs

        self.perc_diff = self.rel_diff * 100.0
        self.perc_diff_human = self.rel_diff_human * 100.0

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "MetricValueDiff(abs={}, rel={})".format(self.abs_diff, self.rel_diff)


class SbsMetricResult(object):
    def __init__(self, metric_key, sbs_metric_values):
        """
        :type metric_key: PluginKey
        :type sbs_metric_values: SbsMetricValues
        """
        self.metric_key = metric_key
        self.sbs_metric_values = sbs_metric_values

    def serialize(self):
        result = OrderedDict()
        result["metric_key"] = self.metric_key.serialize()
        result["sbs_metric_values"] = self.sbs_metric_values.serialize()
        return result

    @staticmethod
    def deserialize(result_data):
        if result_data is None:
            return None
        metric_key = PluginKey.deserialize(result_data["metric_key"])
        sbs_metric_values = SbsMetricValues.deserialize(result_data["sbs_metric_values"])
        return SbsMetricResult(
            metric_key=metric_key,
            sbs_metric_values=sbs_metric_values,
        )

    def key(self):
        return self.metric_key


class SbsMetricValues(object):
    def __init__(self, single_results=None, pair_results=None, custom_results=None):
        """
        :type single_results: list[dict]
        :type pair_results: list[dict]
        :type custom_results:
        """
        self.single_results = single_results
        self.pair_results = pair_results
        self.custom_results = custom_results

    def serialize(self):
        result = OrderedDict()
        if self.single_results:
            result["single_results"] = self.single_results
        if self.pair_results:
            result["pair_results"] = self.pair_results
        if self.custom_results:
            result["custom_results"] = self.custom_results
        return result

    @staticmethod
    def deserialize(values_data):
        """
        :type values_data: dict | None
        :rtype: SbsMetricValues
        """
        if values_data is None:
            return None
        single_results = values_data.get("single_results")
        pair_results = values_data.get("pair_results")
        custom_results = values_data.get("custom_results")
        return SbsMetricValues(single_results=single_results,
                               pair_results=pair_results,
                               custom_results=custom_results)


class LampResult(object):
    def __init__(self, lamp_key, lamp_values):
        """
        :type lamp_key: LampKey
        :type lamp_values: list[MetricResult]
        """

        self.lamp_key = lamp_key
        self.lamp_values = lamp_values
        self.coloring = MetricColoring.LAMP

    def serialize(self):
        result = OrderedDict()

        result["lamp_key"] = self.lamp_key.serialize()
        result["coloring"] = self.coloring

        lamp_values = [value.serialize() for value in self.lamp_values]
        result["values"] = lamp_values

        return result

    @staticmethod
    def deserialize(lamp_result_data):
        """
        :type lamp_result_data: dict
        :rtype: LampResultResult
        """
        lamp_key = LampKey.deserialize(lamp_result_data.get("lamp_key"))

        values_data = lamp_result_data.get("values", [])
        lamp_values = [MetricResult.deserialize(data) for data in values_data]

        coloring = lamp_result_data.get("coloring")
        assert coloring == MetricColoring.LAMP

        if coloring not in MetricColoring.ALL:
            raise PoolException("Incorrect coloring in metric result: {}".format(coloring))

        lamp_result = LampResult(
            lamp_key=lamp_key,
            lamp_values=lamp_values,
        )

        return lamp_result

    def __str__(self):
        return "LampRes({}, {}, {})".format(self.lamp_key, self.coloring, self.lamp_values)

    def __repr__(self):
        return str(self)

    def __eq__(self, other):
        raise Exception("LampResult is not comparable")

    def __hash__(self):
        raise Exception("LampResult is not hashable")
