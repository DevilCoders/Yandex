import logging
from collections import OrderedDict
from collections import defaultdict

import yaqutils.misc_helpers as umisc
import yaqutils.six_helpers as usix
from experiment_pool import MetricResult  # noqa
from experiment_pool import MetricStats
from experiment_pool import PoolException
from user_plugins import PluginKey  # noqa


class Experiment(object):
    def __init__(self, testid=None, serpset_id=None, sbs_system_id=None,
                 metric_results=None, extra_data=None, errors=None):
        """
        :type testid: str | None
        :type serpset_id: str | None
        :type sbs_system_id: str | None
        :type metric_results: list[MetricResult] | None
        :type extra_data: object | None
        :type errors: list[str] | None
        """
        if testid is None and serpset_id is None:
            raise PoolException("Either testid or serpset_id should be specified. ")

        self.testid = umisc.optional_non_empty_string(testid)

        self.serpset_id = umisc.optional_non_empty_string(serpset_id)
        # typical serpset ID is a number like '20301341'.
        # if it's too large => repo
        if serpset_id is not None and len(serpset_id) > 40:
            raise PoolException("Serpset ID seems to be incorrect (too long): {}".format(serpset_id))

        self.sbs_system_id = umisc.optional_non_empty_string(sbs_system_id)

        if metric_results is None:
            metric_results = []
        self.metric_results = metric_results
        """:type: list[MetricResult]"""

        self.extra_data = extra_data
        if errors is None:
            errors = []
        self.errors = errors
        """:type: list[str]"""

    def clone(self, clone_metric_results=False):
        """
        :type clone_metric_results: bool
        :rtype: Experiment
        """
        metric_results = self.metric_results if clone_metric_results else None
        return Experiment(testid=self.testid,
                          serpset_id=self.serpset_id,
                          sbs_system_id=self.sbs_system_id,
                          extra_data=self.extra_data,
                          errors=self.errors,
                          metric_results=metric_results)

    @staticmethod
    def build_metric_results_map(metric_results):
        """
        :type: list[MetricResult]
        :rtype: dict[PluginKey, MetricResult]
        """
        return {metric_res.key(): metric_res for metric_res in metric_results}

    def get_metric_results_map(self):
        """
        :rtype: dict[PluginKey, MetricResult]
        """
        return self.build_metric_results_map(self.metric_results)

    def find_metric_result(self, metric_key):
        """
        :type metric_key: PluginKey
        :return:
        """
        return self.get_metric_results_map().get(metric_key)

    def all_metric_keys(self):
        """
        :rtype: set[PluginKey]
        """
        metric_keys = set()
        for metric_result in self.metric_results:
            metric_keys.add(metric_result.metric_key)
        return metric_keys

    def all_criteria_keys(self):
        """
        :rtype: set[PluginKey]
        """
        criteria_keys = set()
        for metric_result in self.metric_results:
            criteria_keys.update(metric_result.all_criteria_keys())
        return criteria_keys

    def all_data_types(self):
        data_types = set()
        for metric_result in self.metric_results:
            if metric_result.metric_values.data_file is not None:
                data_types.add(metric_result.metric_values.data_type)
        if len(data_types) > 1:
            logging.warning("Experiment {} has mixed data types", self)
        return data_types

    def serialize(self):
        """
        :type self: Experiment
        :return: dict
        """
        result = OrderedDict()

        if self.testid is not None:
            result["testid"] = self.testid

        if self.serpset_id is not None:
            result["serpset_id"] = self.serpset_id

        if self.sbs_system_id is not None:
            result["sbs_system_id"] = self.sbs_system_id

        if self.metric_results:
            result["metrics"] = umisc.serialize_array(self.metric_results)

        if self.errors:
            result["errors"] = self.errors

        if self.extra_data:
            result["extra_data"] = self.extra_data

        return result

    def add_error(self, error_type):
        """
        :type error_type: str
        """
        self.errors.append(error_type)

    # this method is faster if we have lots of metric results.
    def add_metric_results(self, metric_results):
        """
        :type metric_results: list[MetricResult]
        :rtype: None
        """
        existing_keys = set(mr.metric_key for mr in self.metric_results)

        for result in metric_results:
            if result.metric_key in existing_keys:
                raise PoolException("Duplicate metric {} in exp {}".format(result.metric_key, self))
            # use direct access to metric_results in favor of speed.
            self.metric_results.append(result)
            existing_keys.add(result.metric_key)

    def clear_metric_results(self):
        self.metric_results = []

    def add_metric_result(self, metric_result):
        """
        :type metric_result: MetricResult
        :rtype:
        """
        new_key = metric_result.metric_key
        for index, mr in enumerate(self.metric_results):
            if new_key == mr.metric_key:
                raise PoolException("Metric {} in exp {} already exists at index {}".format(new_key, self, index))
        self.metric_results.append(metric_result)

    def __str__(self):
        return "Exp(testid={}, serpset={})".format(self.testid, self.serpset_id)

    def __repr__(self):
        return str(self)

    def key(self):
        # this key should identify metric calculation
        return self.testid, self.serpset_id or "", repr(self.extra_data or "")

    def __eq__(self, other):
        raise PoolException("Experiment is not comparable")

    def __cmp__(self, other):
        raise PoolException("Experiment is not comparable")

    def __hash__(self):
        raise PoolException("Experiment is not hashable")

    # next two methods returns one testid/serpset_id in most cases,
    # and empty set if except if some errors occured.
    # these methods added for similarity with Observation and Pool classes.
    def all_testids(self, include_errors=False):
        if not include_errors and self.errors:
            return tuple()
        if self.testid is None:
            return tuple()
        return self.testid,

    def has_errors(self):
        return set(self.errors) & ExpErrorType.SERP_ERRORS

    def all_serpset_ids(self, include_errors=False):
        if not include_errors:
            if self.has_errors():
                return tuple()
        if self.serpset_id is None:
            return tuple()
        return self.serpset_id,

    def get_metric_stats(self):
        """
        :rtype: dict[PluginKey, MetricStats]
        """
        detailed_stats = defaultdict(list)
        for metric_key, metric_result in usix.iteritems(self.get_metric_results_map()):
            detailed_stats[metric_key].append(metric_result.metric_values.significant_value)

        return MetricStats.aggregate_stats(detailed_stats)

    def sort(self):
        for metric_result in self.metric_results:
            metric_result.sort()
        self.metric_results.sort(key=lambda x: x.key())


# noinspection PyClassHasNoInit
class ExpErrorType:
    SERP_FETCH = "SERP_FETCH"
    SERP_ERRORS = {SERP_FETCH}
