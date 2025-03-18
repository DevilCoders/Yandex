#!/usr/bin/env python3

import argparse
import logging

from serp import SerpFetchParams
from serp import RawSerpDataStorage
from serp import ParsedSerpDataStorage
from serp import MetricDataStorage
from serp import McCalcOptions

import serp.serp_helpers as shelp
import serp.mc_composite as smp

import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqutils.time_helpers as utime
import yaqutils.nirvana_helpers as unirv

import experiment_pool.pool_helpers as phelp
import mstand_metric_helpers.common_metric_helpers as mhelp


def parse_args():
    parser = argparse.ArgumentParser(description="Metrics composite computation pipeline")
    uargs.add_generic_params(parser)

    uargs.add_threads(parser)
    mstand_uargs.add_storage_params(parser)

    # build pool stage
    mstand_uargs.add_serpset_id(parser, required=False)
    # debug option: serpset filter
    mstand_uargs.add_serpset_id_filter(parser, required=False)
    mstand_uargs.add_input_pool(parser, required=False)

    # fetch stage
    mstand_uargs.add_serp_fetch_params(parser, use_external_convertor_default=True)
    mstand_uargs.add_mc_serpsets_tar(parser)

    # common fetch + parse stage
    mstand_uargs.add_serp_attributes(parser)
    # parse stage
    mstand_uargs.add_serp_parse_params(parser)

    # calc stage
    mstand_uargs.add_offline_metric_calc_options(parser, def_use_internal_output=False,
                                                 def_use_external_output=True, def_gzip_mc_output=True)

    return parser.parse_args()


def show_local_replay_help(cli_args):
    arc_path = "https://a.yandex-team.ru/arc/trunk/arcadia/tools/mstand"

    inputs = "--mc-serpsets-tar out_serpsets_data_gz.tar --batch metric-batch.json --source metrics.tar.gz"
    flags = "--allow-broken-components --skip-metric-errors"
    # --remove-raw-serpsets is not very good for debug reasons
    cmd_line = "./offline-mc-composite.py {} {}".format(flags, inputs)

    if cli_args.threads:
        cmd_line += " --threads {}".format(cli_args.threads)

    if cli_args.serp_attrs:
        cmd_line += " --serp-attrs {}".format(cli_args.serp_attrs)

    if cli_args.component_attrs:
        cmd_line += " --component-attrs {}".format(cli_args.component_attrs)

    if cli_args.sitelink_attrs:
        cmd_line += " --sitelink-attrs {}".format(cli_args.sitelink_attrs)

    if cli_args.judgements:
        cmd_line += " --judgements {}".format(cli_args.judgements)

    lines = [
        "",
        "",
        "==== MSTAND OFFLINE LOCAL DEBUG INSTUCTIONS: ====",
        "To replay this run locally:",
        "1. checkout mstand from {}".format(arc_path),
        "2. download serpset archive, metric sources and batch from Nirvana workflow",
        "3. run this script from mstand folder (you may also add params '--no-use-cache --remove-raw-serpsets')",
        ""
    ]

    log_msg = "\n".join(lines)
    logging.info(log_msg)
    log_cmd_line_safe(cmd_line)


def log_cmd_line_safe(cmd_line):
    # nirvana has max log line about 2k characters
    max_log_len = 1500
    while cmd_line:
        cmd_line_part = cmd_line[:max_log_len]
        cmd_line = cmd_line[max_log_len:]
        if cmd_line:
            cmd_line_part += "\\"
        logging.info("%s", cmd_line_part)


def main_worker(cli_args):
    umisc.configure_logger(verbose=cli_args.verbose, quiet=cli_args.quiet)

    if not any([cli_args.input_file, cli_args.mc_serpsets_tar, cli_args.serpset_id]):
        raise Exception("You should specify one of params: mstand pool, serpset-tar or serpset-id list.")

    if cli_args.input_file:
        pool = phelp.load_pool(cli_args.input_file)
    elif cli_args.serpset_id:
        logging.info("Building pool from serpset list")
        pool = shelp.build_mc_pool(cli_args.serpset_id)
    else:
        logging.info("Using serpset TAR archive as serpset list")
        pool = None

    mc_serpsets_tar = cli_args.mc_serpsets_tar

    if mc_serpsets_tar:
        logging.info("Using external serpset .tgz archive")

    show_local_replay_help(cli_args)

    fetch_params = SerpFetchParams.from_cli_args(cli_args)

    raw_serp_storage = RawSerpDataStorage.from_cli_args(cli_args)
    parsed_serp_storage = ParsedSerpDataStorage.from_cli_args(cli_args)
    metric_storage = MetricDataStorage.from_cli_args(cli_args)
    metric_container = mhelp.create_container_from_cli_args(cli_args, skip_broken_metrics=cli_args.skip_metric_errors)

    env_workflow_id = unirv.get_nirvana_workflow_uid()

    yt_output_ttl = utime.human_readable_time_period_to_seconds(cli_args.mc_yt_output_ttl)

    mc_calc_options = McCalcOptions(skip_metric_errors=cli_args.skip_metric_errors,
                                    use_internal_output=cli_args.use_internal_output,
                                    use_external_output=cli_args.use_external_output,
                                    collect_scale_stats=cli_args.collect_scale_stats,
                                    mc_alias_prefix=cli_args.mc_alias_prefix,
                                    mc_error_alias_prefix=cli_args.mc_error_alias_prefix,
                                    skip_failed_serps=cli_args.skip_failed_serps,
                                    numeric_values_only=cli_args.numeric_values_only,
                                    gzip_mc_output=cli_args.gzip_mc_output,
                                    remove_raw_serpsets=cli_args.remove_raw_serpsets,
                                    allow_no_position=cli_args.allow_no_position,
                                    allow_broken_components=cli_args.allow_broken_components,
                                    env_workflow_id=env_workflow_id,
                                    calc_mode=cli_args.calc_mode,
                                    yt_auth_token_file=cli_args.yt_auth_token_file,
                                    mc_output_file=cli_args.mc_output,
                                    mc_output_mr_table=cli_args.mc_output_mr_table,
                                    mc_broken_metrics_file=cli_args.mc_broken_metrics,
                                    yt_pool=cli_args.yt_pool,
                                    yt_cluster=cli_args.yt_cluster,
                                    yt_root_path=cli_args.mc_yt_root_path,
                                    yt_output_ttl=yt_output_ttl)

    smp.mc_composite_main(pool=pool,
                          fetch_params=fetch_params,
                          mc_serpsets_tar=mc_serpsets_tar,
                          metric_container=metric_container,
                          threads=cli_args.threads,
                          raw_serp_storage=raw_serp_storage,
                          parsed_serp_storage=parsed_serp_storage,
                          metric_storage=metric_storage,
                          pool_output=cli_args.output_file,
                          save_to_tar=cli_args.save_to_tar,
                          mc_calc_options=mc_calc_options)


def main():
    cli_args = parse_args()
    unirv.run_and_save_exception(main_worker, cli_args)


if __name__ == "__main__":
    main()
