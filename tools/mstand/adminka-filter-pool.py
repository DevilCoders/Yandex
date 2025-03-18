#!/usr/bin/env python3

import argparse

import adminka.activity
import adminka.filter_fetcher
import adminka.testid
import experiment_pool.pool_helpers as pool_helpers
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
from adminka.filter_pool import PoolFilter, trim_days, ensure_full_days_all, split_at_salt_changes_all, split_overlap


def parse_args():
    parser = argparse.ArgumentParser(description="mstand filter pool")
    uargs.add_verbosity(parser)
    mstand_uargs.add_input_pool(parser)
    uargs.add_output(parser)

    PoolFilter.add_cli_args(parser)

    parser.add_argument(
        "--trim-from",
        type=int,
        help="trim dates from N-th day"
    )
    parser.add_argument(
        "--trim-to",
        type=int,
        help="trim dates to N-th day"
    )
    parser.add_argument(
        "--keep-truncated",
        action="store_true",
        help="keep observations with duration less than (trim-to - trim-from + 1)"
    )
    parser.add_argument(
        "--ensure-full-days",
        action="store_true",
        help="make sure all observations in the pool only contain full days"
    )
    uargs.add_boolean_argument(
        parser,
        "--remove-duplicates",
        help_message="remove duplicate observations from pool"
    )
    uargs.add_boolean_argument(
        parser,
        "--split-at-salt-changes",
        help_message="split observations on salt change dates"
    )
    parser.add_argument(
        "--split-overlap-length",
        type=int,
        help="overlap splitting method (convert [a,b] to [a,a+LENGTH-1],[a+STEP,a+LENGTH-1+STEP],...)"
    )
    parser.add_argument(
        "--split-overlap-step",
        type=int,
        default=1,
        help="overlap splitting method (convert [a,b] to [a,a+LENGTH-1],[a+STEP,a+LENGTH-1+STEP],...)"
    )

    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    pool = pool_helpers.load_pool(cli_args.input_file)

    if cli_args.remove_salt_changes and cli_args.split_at_salt_changes:
        raise Exception("--split-at-salt-changes and --remove-salt-changes can't be used at the same time")

    pool_filter = PoolFilter.from_cli_args(cli_args)
    pool = pool_filter.filter(pool)

    if cli_args.split_overlap_length and cli_args.split_overlap_step:
        split_overlap(pool.observations, cli_args.split_overlap_length, cli_args.split_overlap_step)

    if cli_args.trim_from or cli_args.trim_to:
        trim_days(pool.observations, cli_args.trim_from, cli_args.trim_to, cli_args.keep_truncated)

    if cli_args.ensure_full_days:
        pool.observations = ensure_full_days_all(pool.observations, pool_filter.session)

    if cli_args.remove_duplicates:
        adminka.filter_fetcher.fetch_all(pool, allow_bad_filters=True)
        pool = pool_helpers.remove_duplicates(pool)

    if cli_args.split_at_salt_changes:
        pool.observations = split_at_salt_changes_all(pool.observations, pool_filter.session)

    pool_filter.session.log_stats()
    pool_helpers.dump_pool(pool, cli_args.output_file)
    pool.log_stats()
    pool_filter.log_enum_field_stats()


if __name__ == "__main__":
    main()
