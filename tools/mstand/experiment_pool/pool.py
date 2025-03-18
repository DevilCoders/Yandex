import logging
from collections import OrderedDict
from collections import defaultdict

import yaqutils.misc_helpers as umisc
import yaqutils.six_helpers as usix
from experiment_pool import Experiment  # noqa
from experiment_pool import MetricStats
from experiment_pool import Observation  # noqa
from experiment_pool.metric_result import SbsMetricResult
from experiment_pool.meta import Meta  # noqa


class Pool(object):
    def __init__(
        self,
        observations=None,
        synthetic_summaries=None,
        extra_data=None,
        lamps=None,
        meta=None,
    ):
        """
        :type observations: list[Observation]
        :type synthetic_summaries: list[SyntheticSummary]
        :type extra_data: dict[str]
        :type lamps: list[LampResult]
        :type meta: Optional[Meta]
        """
        observations = observations or []
        assert isinstance(observations, list), "Pool.observations is expected to be a list"
        self.observations = observations

        self.synthetic_summaries = synthetic_summaries
        self.metric_stats = None
        self.extra_data = extra_data

        if lamps is None:
            lamps = []
        self.lamps = lamps
        self.meta = meta

    def serialize(self):
        """
        :rtype: dict
        """
        serialized = OrderedDict()
        serialized["version"] = 1

        if self.synthetic_summaries is not None:
            serialized["synthetic_summaries"] = [s_sum.serialize() for s_sum in self.synthetic_summaries]

        serialized["observations"] = [obs.serialize() for obs in self.observations]

        if self.metric_stats:
            metric_stats = {key.pretty_name(): mstats.serialize() for key, mstats in usix.iteritems(self.metric_stats)}
            serialized["metric_stats"] = metric_stats

        if self.extra_data:
            serialized["extra_data"] = self.extra_data

        if self.lamps:
            serialized["lamps"] = [lamp.serialize() for lamp in self.lamps]

        if self.meta:
            serialized["meta"] = self.meta.serialize()

        return serialized

    def fill_sbs_workflow_ids(self, sbs_ticket_workflow_map=None):
        # hide requests/urllib imports from YT
        import sbs.samadhi
        if sbs_ticket_workflow_map is None:
            sbs_ticket_workflow_map = {}

        for obs in self.observations:
            if obs.sbs_ticket and not obs.sbs_workflow_id:
                if obs.sbs_ticket not in sbs_ticket_workflow_map:
                    sbs_ticket_workflow_map[obs.sbs_ticket] = sbs.samadhi.get_sbs_workflow_id(obs.sbs_ticket)
                obs.sbs_workflow_id = sbs_ticket_workflow_map[obs.sbs_ticket]

    def get_sbs_ticket_workflow_pairs(self):
        """
        :rtype: dict[str, str]
        """
        result_json = []
        for obs in self.observations:
            if obs.sbs_ticket and obs.sbs_workflow_id:
                ticket_workflow_pair = {
                    "st-ticket": obs.sbs_ticket,
                    "workflow-id": obs.sbs_workflow_id,
                }
                result_json.append(ticket_workflow_pair)
        return result_json

    def attach_sbs_metric_results(self, sbs_metric_results_json):
        """
        :type sbs_metric_results_json: dict
        """
        metric_results_map = {}
        for result_json in sbs_metric_results_json:
            sbs_ticket = result_json["st-ticket"]
            sbs_workflow_id = result_json["workflow-id"]
            sbs_metric_results = umisc.deserialize_array(result_json["sbs_metric_results"], SbsMetricResult)
            metric_results_map[(sbs_ticket, sbs_workflow_id)] = sbs_metric_results
        for obs in self.observations:
            if obs.sbs_ticket and obs.sbs_workflow_id:
                obs.sbs_metric_results = metric_results_map.get((obs.sbs_ticket, obs.sbs_workflow_id))

    def all_serpset_ids(self, include_control=True, include_errors=False):
        """
        :type include_control: bool
        :type include_errors: bool
        :rtype: list[str]
        """
        serpset_ids = set()
        for obs in self.observations:
            serpset_ids.update(obs.all_serpset_ids(include_control=include_control, include_errors=include_errors))
        return list(sorted(serpset_ids))

    def has_valid_serpsets(self):
        return len(self.all_serpset_ids()) > 0

    def all_testids(self, include_control=True, include_errors=False):
        testids = set()
        for obs in self.observations:
            testids.update(obs.all_testids(include_control, include_errors))
        return sorted(testids)

    def all_experiments(self, include_control=True):
        """
        :type include_control: bool
        :rtype: list[Experiment]
        """
        all_exps = []
        for obs in self.observations:
            all_exps.extend(obs.all_experiments(include_control=include_control))
        return all_exps

    def all_data_types(self):
        data_types = set()
        for obs in self.observations:
            data_types |= obs.all_data_types()
        return data_types

    def all_metric_keys(self):
        metric_keys = set()
        for obs in self.observations:
            metric_keys.update(obs.all_metric_keys())
        return metric_keys

    def all_criteria_keys(self):
        criteria_keys = set()
        for obs in self.observations:
            criteria_keys.update(obs.all_criteria_keys())
        return criteria_keys

    def log_stats(self):
        logging.info("Pool info:")
        logging.info("  Observations: %d", len(self.observations))
        total_testids = len(self.all_testids(include_errors=True))
        good_testids = len(self.all_testids())

        logging.info("  Testids: %d good, %d total", good_testids, total_testids)
        total_serpsets = len(self.all_serpset_ids(include_errors=True))
        good_serpsets = len(self.all_serpset_ids())
        logging.info("  Serpsets: %d good, %d total", good_serpsets, total_serpsets)
        obs_begins = [obs.dates.start for obs in self.observations if obs.dates.start]
        obs_date_min = min(obs_begins) if obs_begins else None
        obs_ends = [obs.dates.end for obs in self.observations if obs.dates.end]
        obs_date_max = max(obs_ends) if obs_ends else None
        logging.info("Pool overall observation date range: [%s:%s]", obs_date_min, obs_date_max)

    def clear_metric_results(self):
        logging.info("Clearing metric results in pool")
        all_exps = self.all_experiments()
        for exp in all_exps:
            exp.clear_metric_results()

    def get_metric_stats(self):
        detailed_stats = defaultdict(list)
        for obs in self.observations:
            obs_stats = obs.get_metric_stats()
            for metric_key, metric_stats in usix.iteritems(obs_stats):
                detailed_stats[metric_key].extend(metric_stats.all_values())

        return MetricStats.aggregate_stats(detailed_stats)

    def save_metric_stats(self):
        self.metric_stats = self.get_metric_stats()

    def log_metric_stats(self):
        stats = self.get_metric_stats()
        logging.info("Metric values stats for entire pool: ")
        for metric_key, metric_stats in usix.iteritems(stats):
            logging.info("--> Metric %s: %s", metric_key.pretty_name(), metric_stats)

    def sort(self, sort_exp=True):
        for observation in self.observations:
            observation.sort(sort_exp)
        self.observations.sort(key=lambda x: x.key())

    def init_services(self, services):
        for observation in self.observations:
            observation.services = services


class ColoringInfo(object):
    def __init__(self, total_count, colorings, sensitivity):
        """
        :type total_count: int
        :type colorings: list[dict]
        :type sensitivity: float
        """
        self.total_count = total_count
        self.colorings = colorings
        self.sensitivity = sensitivity

    def serialize(self):
        result = OrderedDict()
        result["total_count"] = self.total_count
        result["colorings"] = [coloring for coloring in self.colorings]
        result["sensitivity"] = self.sensitivity
        return result

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "ColoringInfo(total_count={}, sens={}, colorings={}".format(self.total_count,
                                                                           self.sensitivity,
                                                                           self.colorings)


@umisc.hash_and_ordering_from_key_method
class SyntheticSummary(object):
    def __init__(self, metric_key, criteria_key, coloring_info):
        """
        :type metric_key: MetricKey
        :type criteria_key: CriteriaKey
        :type coloring_info: ColoringInfo
        """
        self.metric_key = metric_key
        self.criteria_key = criteria_key
        # value part
        self.coloring_info = coloring_info

    def serialize(self):
        result = OrderedDict()
        result["metric_key"] = self.metric_key.serialize()
        result["criteria_key"] = self.criteria_key.serialize()
        result["coloring_info"] = self.coloring_info.serialize()
        return result

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "SynthSummary(metric={}, crit={}, col_info={}".format(self.metric_key,
                                                                     self.criteria_key,
                                                                     self.coloring_info)

    def key(self):
        return self.metric_key, self.criteria_key
