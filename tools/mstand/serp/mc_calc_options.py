import logging

from mstand_enums.mstand_offline_enums import OfflineCalcMode
from mstand_utils import OfflineDefaultValues


class McCalcOptions(object):
    def __init__(self, skip_metric_errors=False, use_internal_output=False, use_external_output=True, collect_scale_stats=True,
                 mc_alias_prefix="metric.", mc_error_alias_prefix="metricError.",
                 skip_failed_serps=False, numeric_values_only=False, gzip_mc_output=False,
                 allow_broken_components=False, remove_raw_serpsets=False,
                 allow_no_position=True, calc_mode=OfflineCalcMode.LOCAL, env_workflow_id=None, yt_auth_token_file=None,
                 mc_output_file=None, mc_output_mr_table=None, mc_broken_metrics_file=None,
                 serpset_id_filter=None,
                 yt_pool=OfflineDefaultValues.DEF_YT_POOL, yt_cluster=None,
                 yt_root_path=OfflineDefaultValues.DEF_YT_PATH,
                 yt_output_ttl=OfflineDefaultValues.DEF_YT_OUTPUT_TTL):
        self.skip_metric_errors = skip_metric_errors
        self.use_internal_output = use_internal_output
        self.use_external_output = use_external_output
        self.collect_scale_stats = collect_scale_stats
        if mc_alias_prefix == mc_error_alias_prefix:
            raise Exception("Misconfig: metric alias prefix and metric error alias prefix could not be the same.")

        self.mc_alias_prefix = mc_alias_prefix
        self.mc_error_alias_prefix = mc_error_alias_prefix
        self.skip_failed_serps = skip_failed_serps
        self.numeric_values_only = numeric_values_only
        self.gzip_mc_output = gzip_mc_output

        self.allow_broken_components = allow_broken_components
        self.remove_raw_serpsets = remove_raw_serpsets
        self.allow_no_position = allow_no_position
        self.calc_mode = calc_mode
        self.env_workflow_id = env_workflow_id
        self.yt_auth_token_file = yt_auth_token_file
        self.mc_output_file = mc_output_file
        self.mc_output_mr_table = mc_output_mr_table
        self.mc_broken_metrics_file = mc_broken_metrics_file
        self.serpset_id_filter = serpset_id_filter
        self.yt_cluster = yt_cluster
        if yt_pool is None:
            yt_pool = OfflineDefaultValues.DEF_YT_POOL
        logging.info("YT Pool is not specified, using default: %s", yt_pool)
        self.yt_pool = yt_pool

        self.yt_root_path = yt_root_path
        # seconds
        self.yt_output_ttl = yt_output_ttl

        logging.info("skip metric errors: %s, numeric values only: %s, alias_prefix: %s, error_alias_prefix: %s, gzip mc output: %s",
                     self.skip_metric_errors, self.numeric_values_only, self.mc_alias_prefix, self.mc_error_alias_prefix, self.gzip_mc_output)
