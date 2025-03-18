#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse

import adminka.ab_cache
import adminka.ab_helpers
import experiment_pool.pool_helpers as pool_helpers
import session_squeeze_local_single
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
from session_local.squeeze_local import SqueezeBackendLocal
from session_squeezer.squeeze_runner import SqueezeRunner


def parse_args():
    parser = argparse.ArgumentParser(description="Collect data from user_sessions into files for mstand")
    session_squeeze_local_single.add_arguments(parser)
    mstand_uargs.add_input_pool(parser)
    uargs.add_output(parser)
    mstand_uargs.add_ab_token_file(parser)
    mstand_uargs.add_all_users_flag(parser)
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    pool = pool_helpers.load_pool(cli_args.input_file)
    session = adminka.ab_cache.AdminkaCachedApi(auth_token=mstand_uargs.get_token(cli_args.ab_token_file))

    if not cli_args.all_users:
        adminka.ab_helpers.validate_and_enrich(
            pool=pool,
            session=session,
            add_filters=cli_args.use_filters,
            services=cli_args.services,
        )
    else:
        pool = pool_helpers.create_all_users_mode_pool(pool)
        pool.init_services(cli_args.services)

    backend = SqueezeBackendLocal()
    squeeze_runner = SqueezeRunner.from_cli_args(cli_args, backend, use_processes=True)
    new_pool = squeeze_runner.squeeze_pool(pool)
    pool_helpers.dump_pool(new_pool, cli_args.output_file)


if __name__ == "__main__":
    main()
