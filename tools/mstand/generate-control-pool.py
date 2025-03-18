#!/usr/bin/env python2.7

import argparse
import datetime

import experiment_pool.pool_helpers as upool
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqutils.time_helpers as utime
from adminka.control_pool import generate_control_pool


def parse_args():
    parser = argparse.ArgumentParser(description="mstand generate control pool")
    uargs.add_verbosity(parser)
    mstand_uargs.add_dates(parser, required=False)
    mstand_uargs.add_observation_ids(parser)
    uargs.add_output(parser)
    parser.add_argument(
        "--max-range",
        default=14,
        type=int,
        help="period's maximum duration (default=14)",
    )
    uargs.add_boolean_argument(
        parser,
        name="use-split-change-day",
        help_message="add day with split change to the pool (use only if split changes at 00:00)",
        default=False,
    )
    uargs.add_boolean_argument(
        parser,
        name="force-single-days",
        help_message="always add single-day observations for all days without manual split changes",
        default=False,
    )
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    if cli_args.date_to is None:
        end_date = datetime.datetime.now().date()
    else:
        end_date = utime.parse_date_msk(cli_args.date_to)

    if cli_args.date_from is None:
        start_date = end_date - datetime.timedelta(days=91)
    else:
        start_date = utime.parse_date_msk(cli_args.date_from)

    dates = utime.DateRange(start_date, end_date)
    pool = generate_control_pool(
        observations=cli_args.observations,
        dates=dates,
        max_range=cli_args.max_range,
        use_split_change_day=cli_args.use_split_change_day,
        force_single_days=cli_args.force_single_days,
    )
    upool.dump_pool(pool, cli_args.output_file)


if __name__ == "__main__":
    main()
