import logging
import time
import datetime

from collections import defaultdict

import serp.offline_metric_caps as om_caps
import yaqutils.misc_helpers as umisc
import yaqutils.six_helpers as usix

from metrics_api import ExperimentForAPI
from metrics_api import ObservationForAPI
from mstand_enums.mstand_offline_enums import OfflineCalcMode
from mstand_utils import OfflineDefaultValues
from serp import MetricDataStorage  # noqa
from serp import ParsedSerpDataStorage  # noqa
from user_plugins import PluginContainer  # noqa
from user_plugins import PluginKey  # noqa


# global parameters for all offline metric computations
class OfflineGlobalCtx(object):
    def __init__(self, use_internal_output=True, use_external_output=False, load_urls=True, skip_metric_errors=False,
                 collect_scale_stats=True, mc_alias_prefix="metric.", mc_error_alias_prefix="metricError.",
                 skip_failed_serps=False, numeric_values_only=False, gzip_external_output=False,
                 calc_mode=OfflineCalcMode.LOCAL, yt_cluster=OfflineDefaultValues.DEF_YT_CLUSTER,
                 yt_temp_dir=OfflineDefaultValues.DEF_YT_PATH, yt_output_dir=OfflineDefaultValues.DEF_YT_PATH,
                 yt_pool=OfflineDefaultValues.DEF_YT_POOL, env_workflow_id=None,
                 output_yt_table_ttl=OfflineDefaultValues.DEF_YT_OUTPUT_TTL, metric_results_table=None, be_verbose=True):
        """
        :type use_external_output: bool
        :type load_urls: bool
        :type calc_mode: str
        :type yt_cluster: str
        :type be_verbose: bool
        """
        self.use_external_output = use_external_output
        self.use_internal_output = use_internal_output

        if be_verbose:
            logging.info("Global ctx: use_internal_output = %s, use_external_output = %s", self.use_internal_output,
                         self.use_external_output)
        self.load_urls = load_urls
        self.skip_metric_errors = skip_metric_errors
        self.collect_scale_stats = collect_scale_stats
        if mc_alias_prefix == mc_error_alias_prefix:
            raise Exception("Misconfig: metric alias prefix and metric error alias prefix could not be the same.")
        self.mc_alias_prefix = mc_alias_prefix
        self.mc_error_alias_prefix = mc_error_alias_prefix
        self.skip_failed_serps = skip_failed_serps
        self.numeric_values_only = numeric_values_only
        self.gzip_external_output = gzip_external_output

        OfflineCalcMode.validate_enum(calc_mode)
        self.calc_mode = calc_mode
        self.yt_pool = yt_pool
        self.yt_cluster = yt_cluster
        self.yt_temp_dir = yt_temp_dir
        self.yt_output_dir = yt_output_dir
        self.env_workflow_id = env_workflow_id
        self.output_yt_table_ttl = output_yt_table_ttl
        self.metric_results_table = metric_results_table

    def is_auto_calc_mode(self) -> bool:
        return self.calc_mode == OfflineCalcMode.AUTO

    def is_yt_calc_mode(self) -> bool:
        return self.calc_mode == OfflineCalcMode.YT

    def is_local_calc_mode(self) -> bool:
        return self.calc_mode == OfflineCalcMode.LOCAL


# parameters for individual offline metric
class OfflineMetricCtx(object):
    def __init__(self, global_ctx, parsed_serp_storage, metric_storage, plugin_container, be_verbose=True):
        """
        :type global_ctx: OfflineGlobalCtx
        :type parsed_serp_storage: ParsedSerpDataStorage | None
        :type metric_storage: MetricDataStorage | None
        :type plugin_container: PluginContainer
        """
        self.global_ctx = global_ctx
        self.parsed_serp_storage = parsed_serp_storage
        self.metric_storage = metric_storage
        self.plugin_container = plugin_container

        self.container_props = om_caps.validate_metric_capabilities(plugin_container, be_verbose=be_verbose)

        logging.info("%s", self.container_props)

    def get_metric_key_by_id(self, metric_id):
        """
        :type metric_id: int
        :rtype: PluginKey
        """
        return self.plugin_container.plugin_key_map[metric_id]

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "{}".format(self.plugin_container)

    @property
    def skip_failed_serps(self):
        return self.global_ctx.skip_failed_serps


# parameters for individual offline metric computations
@umisc.hash_and_ordering_from_key_method
class OfflineComputationCtx(object):
    def __init__(self, metric_ctx, observation, experiment):
        """
        :type metric_ctx: OfflineMetricCtx
        :type observation: Observation
        :type experiment: Experiment
        """
        self.metric_ctx = metric_ctx
        self.observation = ObservationForAPI(observation.clone())
        self.experiment = ExperimentForAPI(experiment.clone())

        self.error_metrics = set()
        self.metric_calc_times = defaultdict(float)

    def reset_metric_calc_times(self):
        self.metric_calc_times.clear()

    def log_metric_calc_times(self):
        metric_time_table = []
        for metric_id, metric_time in usix.iteritems(self.metric_calc_times):
            metric_time_table.append((metric_id, metric_time))

        metric_time_table.sort(key=lambda x: -x[1])
        logging.info("Metric time table for serpset %s", self.serpset_id)

        total_time = 0
        for row in metric_time_table:
            metric_id, metric_time = row
            metric_timedelta = datetime.timedelta(seconds=metric_time)
            total_time += metric_time
            metric_key = self.get_metric_key_by_id(metric_id)
            logging.info("--> metric %3s: %s %s", metric_id, metric_timedelta, metric_key)
        total_timedelta = datetime.timedelta(seconds=total_time)
        logging.info("Total metrics time for serpset %s: %s", self.serpset_id, total_timedelta)

    def accumulate_metric_calc_time(self, metric_id, start_time):
        """
        :type metric_id: int
        :type start_time: int | float
        :rtype:
        """
        elapsed_time = time.time() - start_time
        assert elapsed_time >= 0
        self.metric_calc_times[metric_id] += elapsed_time

    def get_metric_key_by_id(self, metric_id):
        """
        :type metric_id: int
        :rtype: PluginKey
        """
        return self.metric_ctx.get_metric_key_by_id(metric_id)

    def is_serpset_computed(self):
        metric_storage = self.metric_ctx.metric_storage
        metric_keys = self.metric_ctx.plugin_container.plugin_key_map

        return metric_storage.is_serpset_computed(metric_keys=metric_keys, serpset_id=self.serpset_id,
                                                  use_ext_out=self.use_ext_out, use_int_out=self.use_int_out)

    @property
    def serpset_id(self):
        return self.experiment.serpset_id

    @property
    def skip_failed_serps(self):
        return self.metric_ctx.global_ctx.skip_failed_serps

    @property
    def load_urls(self):
        return self.metric_ctx.global_ctx.load_urls

    @property
    def use_int_out(self):
        return self.metric_ctx.global_ctx.use_internal_output

    @property
    def use_ext_out(self):
        return self.metric_ctx.global_ctx.use_external_output

    @property
    def collect_scale_stats(self):
        return self.metric_ctx.global_ctx.collect_scale_stats

    @property
    def has_detailed(self):
        return self.metric_ctx.container_props.has_detailed

    @property
    def max_serp_depth(self):
        return self.metric_ctx.container_props.max_serp_depth

    @property
    def numeric_values_only(self):
        return self.metric_ctx.global_ctx.numeric_values_only

    def __repr__(self):
        return str(self)

    def key(self):
        return self.experiment.serpset_id

    def __str__(self):
        return "({}, serpset={})".format(self.metric_ctx, self.serpset_id)
