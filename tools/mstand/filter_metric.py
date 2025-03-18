#!/usr/bin/env python3

import argparse

import experiment_pool.pool_helpers as pool_helpers
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
from experiment_pool.filter_metric import MetricFilter


def parse_args():
    parser = argparse.ArgumentParser(description="filter metric result")
    uargs.add_verbosity(parser)
    mstand_uargs.add_input_pool(parser)
    uargs.add_output(parser)

    MetricFilter.add_cli_args(parser)

    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    pool = pool_helpers.load_pool(cli_args.input_file)

    metric_filter = MetricFilter.from_cli_args(cli_args)
    metric_filter.filter_metric_for_pool(pool)

    pool_helpers.dump_pool(pool, cli_args.output_file)
    pool.log_stats()


if __name__ == "__main__":
    main()
