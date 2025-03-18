#!/usr/bin/env python3

import argparse

import mstand_utils.args_helpers as mstand_uargs
import mstand_utils.client_yt as client_yt
import mstand_utils.yt_helpers as mstand_uyt
import yaqutils.args_helpers as uargs
import yaqutils.time_helpers as utime
import yaqutils.misc_helpers as umisc
import yt.wrapper as yt

from mstand_enums.mstand_online_enums import ServiceSourceEnum, ServiceEnum
from mstand_utils.yt_options_struct import YtJobOptions, TableFilterBounds, TableSavingParams


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Download logs for specified service or source")
    uargs.add_verbosity(parser=parser)
    mstand_uargs.add_yt(parser=parser)
    mstand_uargs.add_yt_memory_limit(parser=parser, default=2048)
    mstand_uargs.add_yt_tentative_options(parser=parser)
    mstand_uargs.add_dates(parser=parser, required=True)

    source_group = parser.add_mutually_exclusive_group(required=True)
    source_group.add_argument(
        "--service",
        choices=ServiceEnum.get_all_services(),
        help="service, which logs will be downloaded"
    )
    source_group.add_argument(
        "--source",
        choices=ServiceSourceEnum.ALL,
        help="logs source to be used by downloader"
    )

    TableFilterBounds.add_cli_args(parser=parser)
    TableSavingParams.add_cli_args(parser=parser)
    return parser.parse_args()


def main() -> None:
    cli_args = parse_args()
    umisc.configure_logger(verbose=cli_args.verbose, quiet=cli_args.quiet)

    source = cli_args.source if cli_args.source else ServiceEnum.SOURCES[cli_args.service]

    dates = utime.DateRange.from_cli_args(cli_args=cli_args)
    table_filter_bounds = TableFilterBounds.from_cli_args(cli_args=cli_args)
    table_saving_params = TableSavingParams.from_cli_args(cli_args=cli_args)
    yt_client = yt.YtClient(config=client_yt.create_yt_config_from_cli_args(cli_args))
    mstand_uyt.download_logs(source=source,
                             dates=dates,
                             table_filter_bounds=table_filter_bounds,
                             yt_client=yt_client,
                             yt_options=YtJobOptions.from_cli_args(cli_args),
                             table_saving_params=table_saving_params)


if __name__ == "__main__":
    main()
