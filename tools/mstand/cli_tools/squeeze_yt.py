#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

import experiment_pool.pool_helpers as pool_helpers
import mstand_utils.args_helpers as mstand_uargs
import mstand_utils.client_yt
import mstand_utils.log_helpers as mstand_ulog
import pytlib.client_yt as client_yt
import session_squeezer.services as squeezer_services
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc

from cli_tools import squeeze_yt_single
from mstand_enums.mstand_online_enums import ScriptEnum, ServiceEnum, RectypeEnum
from session_squeezer.squeeze_runner import SqueezeRunner
from session_yt.squeeze_yt import SqueezeBackendYT, YTBackendLocalFiles, YTBackendStatboxFiles


DESCRIPTION = "Collect data from user_sessions into MR table for mstand"


def add_arguments(parser):
    squeeze_yt_single.add_common_arguments(parser)
    mstand_uargs.add_input_pool(parser)
    uargs.add_output(parser)


def main(cli_args):
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    yt_config = mstand_utils.client_yt.create_yt_config_from_cli_args(cli_args, tmpfs=True)

    # merge_pools как мягкая валидация
    pool = pool_helpers.load_and_merge_pools([cli_args.input_file])

    yt_client = client_yt.create_client(server=cli_args.server)
    mstand_ulog.write_yt_log(
        script=ScriptEnum.MSTAND_SQUEEZE,
        rectype=RectypeEnum.START,
        yt_client=yt_client,
        data=dict(
            yt_pool=cli_args.yt_pool,
            pool=pool.serialize(),
            services=cli_args.services,
            enable_nile=cli_args.enable_nile,
            enable_cache=cli_args.enable_cache,
            enable_binary=cli_args.enable_binary,
        ),
    )

    import adminka.ab_cache  # иначе падает на Yt из-за import requests
    session = adminka.ab_cache.AdminkaCachedApi(auth_token=mstand_uargs.get_token(cli_args.ab_token_file))
    import adminka.ab_helpers  # иначе падает на Yt из-за import requests
    adminka.ab_helpers.validate_and_enrich(
        pool=pool,
        session=session,
        add_filters=cli_args.use_filters,
        services=cli_args.services,
        ignore_triggered_testids_filter=cli_args.ignore_triggered_testids_filter,
        allow_bad_filters=squeezer_services.check_allow_any_filters(cli_args.services),
    )

    use_filters = (
        cli_args.use_filters and any(obs.filters for obs in pool.observations)
        or ServiceEnum.check_cache_services(cli_args.services)
    )
    if cli_args.use_yt_resources:
        local_files = YTBackendStatboxFiles(use_filters, cli_args.services)
    else:
        local_files = YTBackendLocalFiles(use_filters, cli_args.services)

    yt_backend = SqueezeBackendYT.from_cli_args(cli_args, local_files, yt_config)
    squeeze_runner = SqueezeRunner.from_cli_args(cli_args, yt_backend)
    new_pool = squeeze_runner.squeeze_pool(pool)
    pool_helpers.dump_pool(new_pool, cli_args.output_file)

    mstand_ulog.write_yt_log(
        script=ScriptEnum.MSTAND_SQUEEZE,
        rectype=RectypeEnum.FINISH,
        yt_client=yt_client,
    )
