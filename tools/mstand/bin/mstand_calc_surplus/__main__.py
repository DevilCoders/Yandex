#!/usr/bin/env python3
import argparse

import yt.wrapper as yt

import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqutils.time_helpers as utime

import mstand_utils.args_helpers as mstand_uargs

from mstand_utils.yt_options_struct import TableBounds
from mstand_utils.yt_options_struct import YtJobOptions

from . import calculate_surplus


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Surplus calculator")

    mstand_uargs.add_yt(parser)
    mstand_uargs.add_yt_data_size_per_job(parser, default=10000)
    mstand_uargs.add_yt_memory_limit(parser, default=4096)
    mstand_uargs.add_lower_reducer_key(parser)
    mstand_uargs.add_upper_reducer_key(parser)
    uargs.add_verbosity(parser)
    parser.add_argument(
        "--day",
        help="date for calculate surplus, "
             "supported formats: YYYY-MM-DD, YYYYMMDD, YYYY-MM-DD-HH-MM-SS etc.",
        type=lambda x: utime.parse_human_readable_datetime_utc(x).date(),
        required=True,
    )
    parser.add_argument(
        "--dest-table",
        help="yt path to result table",
        required=True,
    )

    return parser.parse_args()


def set_config(server):
    yt.config["proxy"]["url"] = server


if __name__ == "__main__":
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)
    set_config(cli_args.server)

    yt_job_options = YtJobOptions(
        pool=cli_args.yt_pool,
        data_size_per_job=cli_args.yt_data_size_per_job,
        memory_limit=cli_args.yt_memory_limit,
    )
    table_bounds = TableBounds(
        lower_reducer_key=cli_args.lower_reducer_key,
        upper_reducer_key=cli_args.upper_reducer_key,
    )
    calculate_surplus.run(
        day=cli_args.day,
        yt_job_options=yt_job_options,
        table_bounds=table_bounds,
        dest_table=cli_args.dest_table,
    )
