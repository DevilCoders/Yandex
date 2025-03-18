#!/usr/bin/env python3
import argparse

import experiment_pool.pool_helpers as pool_helpers
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import reports.plot_metrics as plot_metrics


def parse_args():
    parser = argparse.ArgumentParser(description="Compare metrics results")

    uargs.add_verbosity(parser)
    uargs.add_output(parser, required=True)
    uargs.add_input(parser, help_message="pool(s) with metric results", multiple=True)

    mstand_uargs.add_threshold(parser)
    mstand_uargs.add_output_pool(parser, help_message="write merged pool JSON to this file")

    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    pool = pool_helpers.load_and_merge_pools(cli_args.input_file)

    if cli_args.output_pool:
        pool_helpers.dump_pool(pool, cli_args.output_pool)

    with open(cli_args.output_file, "w") as fd:
        plot_metrics.plot_pool(pool, fd, threshold=cli_args.threshold)


if __name__ == "__main__":
    main()
