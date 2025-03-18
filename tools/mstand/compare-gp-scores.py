#!/usr/bin/env python3

import argparse

import experiment_pool.pool_helpers as pool_helpers
import reports.gp_metric_scores
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc


def parse_args():
    parser = argparse.ArgumentParser(description="Compare metrics results")
    uargs.add_verbosity(parser)
    uargs.add_input(parser, help_message="pool(s) with metric results", multiple=True)
    mstand_uargs.add_output_tsv(parser)
    mstand_uargs.add_output_json(parser)
    mstand_uargs.add_output_pool(parser, help_message="write merged pool to this file")
    mstand_uargs.add_min_pvalue(parser)
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    if not any([
        cli_args.output_json,
        cli_args.output_tsv,
    ]):
        raise Exception("Set at least one of: --output-json/tsv")

    pool = pool_helpers.load_and_merge_pools(cli_args.input_file)

    if cli_args.output_pool:
        pool_helpers.dump_pool(pool, cli_args.output_pool)

    reports.gp_metric_scores.calculate_scores_main(
        pool,
        cli_args.output_tsv,
        cli_args.output_json,
        cli_args.min_pvalue,
    )


if __name__ == "__main__":
    main()
