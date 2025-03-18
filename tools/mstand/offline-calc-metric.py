#!/usr/bin/env python3

import argparse
import logging

import serp.serp_compute_metric as ss_calc
import experiment_pool.pool_helpers as phelp

from serp import ParsedSerpDataStorage
from serp import MetricDataStorage
from serp import OfflineGlobalCtx
from serp import OfflineMetricCtx
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.time_helpers as utime
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqutils.nirvana_helpers as unirv

import mstand_metric_helpers.common_metric_helpers as mhelp


def parse_args():
    parser = argparse.ArgumentParser(description="Compute offline metrics")
    uargs.add_generic_params(parser)

    mstand_uargs.add_load_cache(parser, help_message="load cache with parsed serpsets from this file")

    mstand_uargs.add_storage_params(parser)
    uargs.add_threads(parser)

    mstand_uargs.add_offline_metric_calc_options(parser, def_use_internal_output=True,
                                                 def_use_external_output=False, def_gzip_mc_output=False)

    return parser.parse_args()


def main_worker(cli_args):
    umisc.configure_logger(verbose=cli_args.verbose, quiet=cli_args.quiet)

    skip_metric_errors = cli_args.skip_metric_errors

    metric_container = mhelp.create_container_from_cli_args(cli_args, skip_broken_metrics=skip_metric_errors)

    parsed_serp_storage = ParsedSerpDataStorage.from_cli_args(cli_args)
    metric_storage = MetricDataStorage.from_cli_args(cli_args)

    if cli_args.load_cache:
        logging.info("Extracting parsed serps from cache %s", cli_args.load_cache)
        parsed_serp_storage.unpack_from_tar(cli_args.load_cache)

    pool = phelp.load_pool(parsed_serp_storage.pool_path())

    if not pool.has_valid_serpsets():
        raise Exception("This pool have no valid serpsets (without errors). There is nothing to calculate")

    logging.info("Dropping existing metric results in pool")
    pool.clear_metric_results()

    load_urls = cli_args.load_urls
    env_workflow_id = unirv.get_nirvana_workflow_uid()

    yt_output_ttl = utime.human_readable_time_period_to_seconds(cli_args.mc_yt_output_ttl)

    mc_output_file = cli_args.mc_output
    use_external_output = cli_args.use_external_output

    if use_external_output and not mc_output_file:
        raise Exception("Options conflict: --use-external-output is ON, but --mc-output is empty")

    global_context = OfflineGlobalCtx(use_external_output=cli_args.use_external_output,
                                      use_internal_output=cli_args.use_internal_output,
                                      load_urls=load_urls,
                                      skip_metric_errors=skip_metric_errors,
                                      collect_scale_stats=cli_args.collect_scale_stats,
                                      skip_failed_serps=cli_args.skip_failed_serps,
                                      numeric_values_only=cli_args.numeric_values_only,
                                      calc_mode=cli_args.calc_mode,
                                      env_workflow_id=env_workflow_id,
                                      output_yt_table_ttl=yt_output_ttl,
                                      yt_output_dir=cli_args.mc_yt_root_path,
                                      yt_cluster=cli_args.yt_cluster,
                                      yt_pool=cli_args.yt_pool)

    metric_context = OfflineMetricCtx(global_ctx=global_context,
                                      parsed_serp_storage=parsed_serp_storage,
                                      metric_storage=metric_storage,
                                      plugin_container=metric_container)

    ss_calc.calc_offline_metric_main(pool=pool, metric_ctx=metric_context, threads=cli_args.threads,
                                     yt_auth_token_file=cli_args.yt_auth_token_file)

    # cache's internal pool output
    phelp.dump_pool(pool, cli_args.output_file)

    metric_results_tar_name = cli_args.save_to_tar

    if metric_results_tar_name:
        ss_calc.pack_calculation_results(metric_storage=metric_storage, metric_tar_name=metric_results_tar_name)

    if cli_args.use_external_output:
        # dump metric results on one serpset for Metrics service
        ss_calc.create_external_output(metric_ctx=metric_context, output_file=mc_output_file)

    if cli_args.mc_output_mr_table:
        ss_calc.create_external_output_as_mr_table(global_ctx=global_context, output_file=cli_args.mc_output_mr_table)


def main():
    cli_args = parse_args()
    unirv.run_and_save_exception(main_worker, cli_args)


if __name__ == "__main__":
    main()
