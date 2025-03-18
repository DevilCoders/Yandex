#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

import argparse
import logging

import experiment_pool.pool_helpers as pool_helpers
import mstand_utils.args_helpers as mstand_uargs
import mstand_utils.client_yt
import session_yt.squeeze_enricher
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc

from mstand_enums.mstand_online_enums import ServiceEnum
from session_yt.squeeze_enricher import SqueezeEnricher


def enrich_squeeze_for_pool(cli_args):
    yt_config = mstand_utils.client_yt.create_yt_config_from_cli_args(cli_args)
    services_with_meta = ServiceEnum.convert_aliases(cli_args.services)
    pool = pool_helpers.load_pool(cli_args.input_file)

    import adminka.ab_cache  # иначе падает на Yt из-за import requests
    session = adminka.ab_cache.AdminkaCachedApi(auth_token=mstand_uargs.get_token(cli_args.ab_token_file))
    import adminka.ab_helpers  # иначе падает на Yt из-за import requests
    adminka.ab_helpers.validate_and_enrich(
        pool=pool,
        session=session,
        add_filters=cli_args.use_filters,
        services=cli_args.services,
    )

    logging.info("Enrich squeeze for %d observations", len(pool.observations))
    enricher = SqueezeEnricher.from_cli_args(
        cli_args=cli_args,
        session=session,
    )
    enricher.enrich_observations(
        observations=pool.observations,
        services_with_meta=services_with_meta,
        enrich_keys_raw=cli_args.enrich_keys,
        enrich_values_raw=cli_args.enrich_values,
        replace_existing_fields=cli_args.replace_existing_fields,
        input_table=cli_args.enrich_table_path,
        threads=cli_args.threads,
        yt_pool=cli_args.yt_pool,
        yt_config=yt_config,
    )
    pool_helpers.dump_pool(pool, cli_args.output_file)


def parse_args():
    parser = argparse.ArgumentParser(description="Add new column to squeeze tables for pool")
    SqueezeEnricher.add_cli_args(parser)
    session_yt.squeeze_enricher.add_enrich_squeeze_arguments(parser)
    mstand_uargs.add_input_pool(parser)
    mstand_uargs.add_ab_token_file(parser)
    uargs.add_output(parser)
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)
    enrich_squeeze_for_pool(cli_args)


if __name__ == "__main__":
    main()
