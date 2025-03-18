#!/usr/bin/env python3

import argparse

from typing import Optional

import mstand_utils.args_helpers as mstand_uargs
import mstand_utils.client_yt as client_yt
import mstand_utils.yt_helpers as mstand_uyt
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqutils.time_helpers as utime
import yt.wrapper as yt

from adminka.ab_cache import AdminkaCachedApi
from mstand_enums.mstand_online_enums import ServiceEnum
from mstand_utils.yt_options_struct import YtJobOptions, TableFilterBounds, TableSavingParams
from session_squeezer.squeeze_runner import SqueezeRunner


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Download squeezes for specified service or source")
    uargs.add_verbosity(parser=parser)
    mstand_uargs.add_yt(parser=parser)
    mstand_uargs.add_yt_memory_limit(parser=parser, default=2048)
    mstand_uargs.add_yt_tentative_options(parser=parser)

    mstand_uargs.add_ab_token_file(parser=parser)

    mstand_uargs.add_dates(parser=parser, required=True)
    mstand_uargs.add_list_of_online_services(parser=parser, possible=ServiceEnum.get_all_services())
    mstand_uargs.add_list_of_testids(parser=parser)
    mstand_uargs.add_one_observation_id(parser=parser, required=False)
    mstand_uargs.add_squeeze_path(parser=parser)

    TableFilterBounds.add_cli_args(parser=parser)
    TableSavingParams.add_cli_args(parser=parser)
    return parser.parse_args()


def main() -> None:
    cli_args = parse_args()
    umisc.configure_logger(verbose=cli_args.verbose, quiet=cli_args.quiet)

    filter_hash: Optional[str] = None
    if cli_args.observation_id:
        cli_args.use_filters = True
        cli_args.ignore_triggered_testids_filter = False
        session = AdminkaCachedApi(auth_token=mstand_uargs.get_token(path=cli_args.ab_token_file))
        observation = SqueezeRunner.observation_from_cli_args(cli_args=cli_args, session=session)
        filter_hash = observation.filters.filter_hash

    dates = utime.DateRange.from_cli_args(cli_args=cli_args)
    table_filter_bounds = TableFilterBounds.from_cli_args(cli_args=cli_args)
    table_saving_params = TableSavingParams.from_cli_args(cli_args=cli_args)
    yt_client = yt.YtClient(config=client_yt.create_yt_config_from_cli_args(cli_args))
    services = ServiceEnum.extend_services(services=cli_args.services)
    mstand_uyt.download_squeezes(services=services,
                                 dates=dates,
                                 table_filter_bounds=table_filter_bounds,
                                 yt_client=yt_client,
                                 yt_options=YtJobOptions.from_cli_args(cli_args),
                                 testids=cli_args.testids,
                                 filter_hash=filter_hash,
                                 squeeze_path=cli_args.squeeze_path,
                                 table_saving_params=table_saving_params)


if __name__ == "__main__":
    main()
