#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse

import mstand_utils.args_helpers as mstand_uargs
import session_metric.online_metric_main as om_main
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
from experiment_pool import pool_helpers as phelp
from session_local.metric_local import MetricBackendLocal


def parse_args():
    parser = argparse.ArgumentParser(description="Calculate metric locally for one experiment")
    mstand_uargs.add_common_session_metric_local_args(parser)
    uargs.add_output(parser)
    uargs.add_threads(parser, default=10)
    mstand_uargs.add_ab_token_file(parser)
    mstand_uargs.add_dates(parser)
    mstand_uargs.add_one_testid(parser)
    mstand_uargs.add_save_to_file(parser)
    mstand_uargs.add_use_filters_flag(parser)
    mstand_uargs.add_ignore_triggered_testids_filter(parser)
    mstand_uargs.add_save_to_dir(parser)
    mstand_uargs.add_save_to_tar(parser)
    mstand_uargs.add_all_users_flag(parser)
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    calc_backend = MetricBackendLocal()
    pool = phelp.build_pool_from_cli_args(cli_args)

    om_main.calc_online_metric_main(cli_args, calc_backend, pool)


if __name__ == "__main__":
    main()
