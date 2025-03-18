#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse

import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
from session_local.squeeze_local import SqueezeBackendLocal
from session_squeezer.squeeze_runner import SqueezeRunner


def add_arguments(parser):
    uargs.add_verbosity(parser)
    mstand_uargs.add_use_filters_flag(parser)
    SqueezeRunner.add_cli_args(
        parser=parser,
        default_sessions="./user_sessions",
        default_squeeze="./squeeze",
        default_yuid="./yuid_testids",
        default_yuid_market="",
        default_zen="./logs/zen-stats-log/1d",
        default_zen_sessions="./home/recommender/zen/sessions_aggregates/background",
    )


def parse_args():
    parser = argparse.ArgumentParser(description="Collect data from user_sessions into file")
    add_arguments(parser)
    mstand_uargs.add_ab_token_file(parser)
    mstand_uargs.add_dates(parser)
    mstand_uargs.add_list_of_testids(parser)
    mstand_uargs.add_one_observation_id(parser, required=False)
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    import adminka.ab_cache  # иначе падает на Yt из-за import requests
    session = adminka.ab_cache.AdminkaCachedApi(auth_token=mstand_uargs.get_token(cli_args.ab_token_file))
    observation = SqueezeRunner.observation_from_cli_args(cli_args, session)

    backend = SqueezeBackendLocal()
    squeeze_runner = SqueezeRunner.from_cli_args(cli_args, backend, use_processes=True)
    squeeze_runner.squeeze_observation(observation)


if __name__ == "__main__":
    main()
