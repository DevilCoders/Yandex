import logging
import sys

import serp.serpset_fetcher as ss_fetch
import serp.serpset_parser as ss_parse
import serp.serp_compute_metric as ss_calc
import yaqutils.misc_helpers as umisc

from user_plugins import PluginContainer  # noqa

from serp import MetricDataStorage
from serp import PoolParseContext
from serp import OfflineGlobalCtx
from serp import OfflineMetricCtx
from serp import SerpFetchParams  # noqa
from serp import McCalcOptions  # noqa

import experiment_pool.pool_helpers as phelp
from experiment_pool import Pool  # noqa


def mc_composite_main(pool, metric_container, fetch_params, mc_serpsets_tar, threads, raw_serp_storage, parsed_serp_storage,
                      metric_storage, pool_output, save_to_tar, mc_calc_options):
    """
    :type pool: Pool | None
    :type metric_container: PluginContainer
    :type fetch_params: SerpFetchParams | None
    :type mc_serpsets_tar: str | None
    :type threads: int
    :type raw_serp_storage: RawSerpDataStorage
    :type parsed_serp_storage: ParsedSerpDataStorage
    :type metric_storage: MetricDataStorage
    :type pool_output: str
    :type save_to_tar: str | None
    :type mc_calc_options: McCalcOptions
    :rtype: None
    """

    assert isinstance(metric_storage, MetricDataStorage)

    if mc_calc_options.mc_broken_metrics_file:
        metric_container.dump_broken_plugins(mc_calc_options.mc_broken_metrics_file)

    if mc_serpsets_tar:
        logging.info("external serpsets .tgz input specified, using it.")
        pool = ss_fetch.unpack_mc_serpsets_main(pool=pool, mc_serpsets_tar=mc_serpsets_tar,
                                                raw_serp_storage=raw_serp_storage, convert_threads=threads,
                                                unpack_threads=threads,
                                                use_external_convertor=fetch_params.use_external_convertor,
                                                serpset_id_filter=fetch_params.serpset_id_filter)
    else:
        ss_fetch.fetch_serpsets_main(pool=pool, fetch_params=fetch_params, raw_serp_storage=raw_serp_storage)

    pool.log_stats()

    pool_parse_ctx = PoolParseContext(serp_attrs=fetch_params.serp_attrs,
                                      allow_no_position=mc_calc_options.allow_no_position,
                                      allow_broken_components=mc_calc_options.allow_broken_components,
                                      raw_serp_storage=raw_serp_storage,
                                      parsed_serp_storage=parsed_serp_storage,
                                      threads=threads,
                                      remove_raw_serpsets=mc_calc_options.remove_raw_serpsets)

    ss_parse.parse_serpsets(pool, pool_parse_ctx)

    # pool should not be changed => we may leave it as is
    if not pool.has_valid_serpsets():
        raise Exception("This pool have no valid serpsets (without errors). There is nothing to calculate. ")

    global_ctx = OfflineGlobalCtx(use_external_output=mc_calc_options.use_external_output,
                                  use_internal_output=mc_calc_options.use_internal_output,
                                  load_urls=True,
                                  skip_metric_errors=mc_calc_options.skip_metric_errors,
                                  collect_scale_stats=mc_calc_options.collect_scale_stats,
                                  mc_alias_prefix=mc_calc_options.mc_alias_prefix,
                                  mc_error_alias_prefix=mc_calc_options.mc_error_alias_prefix,
                                  skip_failed_serps=mc_calc_options.skip_failed_serps,
                                  numeric_values_only=mc_calc_options.numeric_values_only,
                                  gzip_external_output=mc_calc_options.gzip_mc_output,
                                  calc_mode=mc_calc_options.calc_mode,
                                  env_workflow_id=mc_calc_options.env_workflow_id,
                                  yt_cluster=mc_calc_options.yt_cluster,
                                  yt_pool=mc_calc_options.yt_pool,
                                  yt_output_dir=mc_calc_options.yt_root_path,
                                  yt_temp_dir=mc_calc_options.yt_root_path,
                                  output_yt_table_ttl=mc_calc_options.yt_output_ttl)

    metric_ctx = OfflineMetricCtx(global_ctx=global_ctx,
                                  parsed_serp_storage=parsed_serp_storage,
                                  metric_storage=metric_storage,
                                  plugin_container=metric_container)

    ss_calc.calc_offline_metric_main(pool=pool, metric_ctx=metric_ctx, threads=threads,
                                     yt_auth_token_file=mc_calc_options.yt_auth_token_file)

    phelp.dump_pool(pool, pool_output)

    if save_to_tar:
        metric_tar_name = save_to_tar
        ss_calc.pack_calculation_results(metric_storage, metric_tar_name)

    if mc_calc_options.mc_output_file:
        # dump metric results on one serpset for Metrics service
        ss_calc.create_external_output(metric_ctx=metric_ctx, output_file=mc_calc_options.mc_output_file)

    if mc_calc_options.mc_output_mr_table:
        ss_calc.create_external_output_as_mr_table(global_ctx=global_ctx, output_file=mc_calc_options.mc_output_mr_table)

    log_disk_usage(metric_storage)
    logging.info("mc composite: all done")


def log_disk_usage(metric_storage):
    """
    :type metric_storage: MetricDataStorage
    """
    logging.info("==== data storage disk usage statistics ====")
    work_dir = metric_storage.base_dir()
    umisc.run_command(["du", "-h", work_dir], stdout_fd=sys.stderr, stderr_fd=sys.stderr)
