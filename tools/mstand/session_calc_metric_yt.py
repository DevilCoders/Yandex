#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse

import mstand_utils.args_helpers as mstand_uargs
import mstand_utils.client_yt
import session_metric.online_metric_main as om_main
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
from experiment_pool import pool_helpers as phelp
from session_yt.metric_yt import MetricBackendYT


def parse_args():
    parser = argparse.ArgumentParser(description="Calculate metric on MR")
    mstand_uargs.add_common_session_metric_yt_args(parser)
    uargs.add_input(parser, help_message="read metric data from this file")
    uargs.add_output(parser)
    uargs.add_threads(parser, default=10)
    uargs.add_boolean_argument(parser, "--allow-empty-tables", default=False,
                               help_message="allow empty tables to be produced")
    mstand_uargs.add_ab_token_file(parser)
    mstand_uargs.add_download_threads(parser, default=10)
    mstand_uargs.add_all_users_flag(parser)
    mstand_uargs.add_ignore_triggered_testids_filter(parser)
    mstand_uargs.add_save_to_dir(parser)
    mstand_uargs.add_save_to_tar(parser)
    mstand_uargs.add_use_filters_flag(parser)
    mstand_uargs.add_yt_memory_limit(parser)
    mstand_uargs.add_yt_data_size_per_job(parser)
    mstand_uargs.add_yt_tentative_options(parser, default_enable=False)
    mstand_uargs.add_use_yt_cli_options(parser)
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    yt_config = mstand_utils.client_yt.create_yt_config_from_cli_args(cli_args)
    calc_backend = MetricBackendYT.from_cli_args(cli_args, yt_config)
    # merge_pools как мягкая валидация
    pool = phelp.load_and_merge_pools([cli_args.input_file])

    om_main.calc_online_metric_main(cli_args, calc_backend, pool)


if __name__ == "__main__":
    main()
