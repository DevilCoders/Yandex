import logging

from collections import OrderedDict
from collections import defaultdict
from typing import List
from typing import Optional
from typing import Tuple

import yaqutils.misc_helpers as umisc
import yaqutils.time_helpers as utime
import yaqutils.six_helpers as usix

from experiment_pool import Experiment  # noqa
from experiment_pool import MetricStats


class ObservationFilters:
    def __init__(
            self,
            filters: Optional[List[Tuple[str, str]]] = None,
            filter_hash: Optional[str] = None,
            has_triggered_testids_filter: bool = False,
    ) -> None:
        self._filter_hash = filter_hash
        self.has_triggered_testids_filter = has_triggered_testids_filter
        if filters:
            self.libra_filters = [(name, value) for name, value in filters]
        else:
            self.libra_filters = []

    def key(self) -> Tuple[Tuple[Tuple[str, str], ...], Optional[str]]:
        return tuple(self.libra_filters), self.filter_hash

    def __str__(self) -> str:
        if self:
            return "ObsFilters(hash={}, [{}])".format(
                self.filter_hash,
                ", ".join("{}: {}".format(name, value) for name, value in self.libra_filters),
            )
        else:
            return "ObsFilters()"

    def __repr__(self) -> str:
        return str(self)

    def __bool__(self) -> bool:
        return bool(self.libra_filters)

    @property
    def filter_hash(self) -> Optional[str]:
        if not self:
            return None
        return self._filter_hash


@umisc.hash_and_ordering_from_key_method
class Observation(object):
    def __init__(self, obs_id, dates, control, sbs_ticket=None, sbs_workflow_id=None, sbs_metric_results=None, experiments=None, filters=None, tags=None, extra_data=None, services=None):
        """
        :type obs_id: str | None
        :type dates: utime.DateRange | None
        :type control: Experiment | None
        :type sbs_ticket: str | None
        :type sbs_workflow_id: str | None
        :type sbs_metric_results: list[experiment_pool.metric_result.SbsMetricResult] | None
        :type experiments: list[Experiment]
        :type filters: ObservationFilters | None
        :type tags: list | None
        :type extra_data: object
        :type services list[str] | None
        """
        self.id = umisc.optional_non_empty_string(obs_id)

        self.dates = dates
        self.control = control

        self.sbs_ticket = sbs_ticket
        self.sbs_workflow_id = sbs_workflow_id
        self.sbs_metric_results = sbs_metric_results

        if experiments is None:
            experiments = []
        self.experiments = experiments

        if filters is None:
            filters = ObservationFilters()

        self.filters = filters
        self.tags = tags

        self.extra_data = extra_data
        self.services = services

    def clone(self, clone_experiments=True, clone_metric_results=False):
        if clone_experiments:
            control = self.control.clone(clone_metric_results=clone_metric_results) if self.control else None
            experiments = [exp.clone(clone_metric_results=clone_metric_results) for exp in self.experiments]
        else:
            control = None
            experiments = []

        return Observation(obs_id=self.id,
                           dates=self.dates,
                           sbs_ticket=self.sbs_ticket,
                           sbs_workflow_id=self.sbs_workflow_id,
                           sbs_metric_results=self.sbs_metric_results,
                           filters=self.filters,
                           tags=self.tags,
                           extra_data=self.extra_data,
                           control=control,
                           experiments=experiments,
                           services=self.services)

    def serialize(self):
        """
        :type self: Observation
        :return: dict
        """
        result = OrderedDict()
        result["observation_id"] = self.id
        if self.dates is not None:
            result["date_from"] = utime.format_date(self.dates.start)
            result["date_to"] = utime.format_date(self.dates.end)

        if self.sbs_ticket is not None:
            result["sbs_ticket"] = self.sbs_ticket
        if self.sbs_workflow_id is not None:
            result["sbs_workflow_id"] = self.sbs_workflow_id
        if self.sbs_metric_results is not None:
            result["sbs_metric_results"] = umisc.serialize_array(self.sbs_metric_results)

        result["control"] = self.control.serialize()
        result["experiments"] = [exp.serialize() for exp in self.experiments]

        if self.extra_data is not None:
            result["extra_data"] = self.extra_data

        if self.tags:
            result["tags"] = self.tags

        return result

    def __str__(self):
        return "Obs(id={}, {})".format(self.id, self.dates)

    def __repr__(self):
        return str(self)

    def key(self):
        return self.key_without_experiments(), self.key_of_experiments()

    def key_without_experiments(self):
        return self.id or "", self.dates, self.sbs_ticket, self.filters.key(), repr(self.extra_data or "")

    def key_of_experiments(self, include_control=True):
        keys = frozenset(exp.key() for exp in self.experiments)
        if include_control:
            return self.control.key(), keys
        return keys

    def all_testids(self, include_control=True, include_errors=False):
        """
        :type include_control: bool
        :type include_errors: bool
        :return: list[str]
        """
        testids = set()
        if include_control and self.control:
            testids |= set(self.control.all_testids(include_errors))
        for experiment in self.experiments:
            testids |= set(experiment.all_testids(include_errors))
        return sorted(testids)

    def all_serpset_ids(self, include_control=True, include_errors=False):
        """
        :type include_control: bool
        :type include_errors: bool
        :return: list[str]
        """
        serpset_ids = set()
        if include_control and self.control:
            serpset_ids |= set(self.control.all_serpset_ids(include_errors))
        for experiment in self.experiments:
            serpset_ids |= set(experiment.all_serpset_ids(include_errors))
        return sorted(serpset_ids)

    def all_experiments(self, include_control=True):
        """
        :type include_control: bool
        :rtype: list[Experiment]
        """
        experiments = []
        if include_control and self.control:
            experiments.append(self.control)
        experiments.extend(self.experiments)
        return experiments

    def all_metric_keys(self, include_control=True):
        metric_keys = set()
        if include_control and self.control:
            metric_keys.update(self.control.all_metric_keys())
        for exp in self.experiments:
            metric_keys.update(exp.all_metric_keys())
        return metric_keys

    def all_data_types(self):
        data_types = set()
        if self.control:
            data_types |= self.control.all_data_types()
        for exp in self.experiments:
            data_types |= exp.all_data_types()

        if len(data_types) > 1:
            logging.info("Observation %s has mixed data types", self)

        return data_types

    def common_metric_keys(self):
        metric_keys = set()
        if self.control:
            metric_keys = self.control.all_metric_keys()
        for exp in self.experiments:
            metric_keys &= exp.all_metric_keys()
        return metric_keys

    def all_criteria_keys(self):
        criteria_keys = set()
        if self.control:
            criteria_keys.update(self.control.all_criteria_keys())
        for exp in self.experiments:
            criteria_keys.update(exp.all_criteria_keys())
        return criteria_keys

    def get_metric_stats(self):
        detailed_stats = defaultdict(list)
        for exp in self.all_experiments():
            exp_stats = exp.get_metric_stats()
            for metric_key, metric_stats in usix.iteritems(exp_stats):
                detailed_stats[metric_key].extend(metric_stats.all_values())

        return MetricStats.aggregate_stats(detailed_stats)

    def sort(self, sort_exp=True):
        for experiment in self.experiments:
            experiment.sort()
        if sort_exp:
            self.experiments.sort(key=lambda x: x.key())
        if self.control:
            self.control.sort()
        if self.sbs_metric_results:
            self.sbs_metric_results.sort(key=lambda x: x.key())

    def remove_testids(self, testids):
        if not testids:
            return
        logging.info("Remove from %s: %s", self, ",".join(testids))
        assert self.control.testid not in testids, "Cannot remove control testid from {}".format(self)
        self.experiments = [experiment for experiment in self.experiments if experiment.testid not in testids]
