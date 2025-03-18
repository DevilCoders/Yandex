#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-
import mstand_utils.args_helpers as mstand_uargs
import mstand_utils.client_yt
import mstand_utils.log_helpers as mstand_ulog
import pytlib.client_yt as client_yt
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc

from mstand_enums.mstand_online_enums import ServiceEnum, ScriptEnum, RectypeEnum
from session_squeezer.squeeze_runner import SqueezeRunner
from session_yt.squeeze_yt import SqueezeBackendYT, YTBackendLocalFiles, YTBackendStatboxFiles


DESCRIPTION = "Collect data from user_sessions into MR table for mstand"


def add_common_arguments(parser):
    uargs.add_verbosity(parser)
    uargs.add_boolean_argument(parser, "--allow-empty-tables", default=False,
                               help_message="allow empty tables to be produced")
    uargs.add_boolean_argument(parser, "--use-yt-resources", inverted_name="--use-local-resources", default=True,
                               help_message="use binaries from yt")
    mstand_uargs.add_yt(parser)
    mstand_uargs.add_yt_memory_limit(parser, default=2048)
    mstand_uargs.add_use_filters_flag(parser)
    mstand_uargs.add_yt_tentative_options(parser)
    mstand_uargs.add_yt_lock_options(parser)
    mstand_uargs.add_sort_threads(parser)
    mstand_uargs.add_ab_token_file(parser)
    mstand_uargs.add_enable_nile(parser)
    mstand_uargs.add_squeeze_bin(parser)
    mstand_uargs.add_yql_token_file(parser)
    mstand_uargs.add_max_data_size_per_job(parser)
    uargs.add_boolean_argument(parser, "--enable-transform", default=False,
                               help_message="enable transform of output tables")
    SqueezeRunner.add_cli_args(parser)


def add_arguments(parser):
    add_common_arguments(parser)
    mstand_uargs.add_dates(parser)
    mstand_uargs.add_list_of_testids(parser)
    mstand_uargs.add_one_observation_id(parser, required=False)


def main(cli_args):
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    from adminka.ab_cache import AdminkaCachedApi
    session = AdminkaCachedApi(auth_token=mstand_uargs.get_token(cli_args.ab_token_file))

    observation = SqueezeRunner.observation_from_cli_args(cli_args, session)
    use_filters = (
        cli_args.use_filters and bool(observation.filters)
        or ServiceEnum.check_cache_services(cli_args.services)
    )

    yt_client = client_yt.create_client(server=cli_args.server)
    mstand_ulog.write_yt_log(
        script=ScriptEnum.MSTAND_SQUEEZE,
        rectype=RectypeEnum.START,
        yt_client=yt_client,
        data=dict(
            yt_pool=cli_args.yt_pool,
            observation=observation.serialize(),
            services=cli_args.services,
            enable_nile=cli_args.enable_nile,
            enable_cache=cli_args.enable_cache,
            enable_binary=cli_args.enable_binary,
        ),
    )

    yt_config = mstand_utils.client_yt.create_yt_config_from_cli_args(cli_args, tmpfs=True)
    if cli_args.use_yt_resources:
        local_files = YTBackendStatboxFiles(use_filters, cli_args.services)
    else:
        local_files = YTBackendLocalFiles(use_filters, cli_args.services)

    yt_backend = SqueezeBackendYT.from_cli_args(cli_args, local_files, yt_config)
    squeeze_runner = SqueezeRunner.from_cli_args(cli_args, yt_backend)
    squeeze_runner.squeeze_observation(observation)

    mstand_ulog.write_yt_log(
        script=ScriptEnum.MSTAND_SQUEEZE,
        rectype=RectypeEnum.FINISH,
        yt_client=yt_client,
    )
