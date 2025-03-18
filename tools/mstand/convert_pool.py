#!/usr/bin/env python3

import argparse

import experiment_pool.pool_helpers as upool

import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc


def parse_args():
    parser = argparse.ArgumentParser(description="mstand convert pool")
    uargs.add_verbosity(parser)
    mstand_uargs.add_input_pool(parser)
    uargs.add_output(parser)
    uargs.add_boolean_argument(parser, "clear-metric-results", default=False, help_message="Clear all metric results")
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)
    pool = upool.load_pool(cli_args.input_file)
    if cli_args.clear_metric_results:
        pool.clear_metric_results()
    upool.dump_pool(pool, cli_args.output_file)


if __name__ == "__main__":
    main()
