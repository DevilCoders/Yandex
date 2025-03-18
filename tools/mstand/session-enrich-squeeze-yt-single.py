#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

import argparse
import logging

import adminka.ab_cache
import mstand_utils.args_helpers as mstand_uargs
import mstand_utils.client_yt
import session_yt.squeeze_enricher
import yaqutils.misc_helpers as umisc
import yaqutils.time_helpers as utime

from experiment_pool import Experiment
from experiment_pool import Observation
from mstand_enums.mstand_online_enums import ServiceEnum
from session_yt.squeeze_enricher import SqueezeEnricher


def enrich_squeeze_single(cli_args):
    yt_config = mstand_utils.client_yt.create_yt_config_from_cli_args(cli_args)
    services_with_meta = ServiceEnum.convert_aliases(cli_args.services)
    date_range = utime.DateRange.from_cli_args(cli_args)

    experiment = Experiment(testid=cli_args.testid)
    observation = Observation(obs_id=cli_args.observation_id, dates=date_range, control=experiment)

    session = adminka.ab_cache.AdminkaCachedApi(auth_token=mstand_uargs.get_token(cli_args.ab_token_file))
    if cli_args.use_filters:
        # avoid requests-related modules import in YT
        import adminka.filter_fetcher as adm_flt_fetch
        adm_flt_fetch.fetch_obs(observation, session)

    logging.info("Enrich squeeze for %s", observation)
    enricher = SqueezeEnricher.from_cli_args(
        cli_args=cli_args,
        session=session,
    )
    enricher.enrich_observations(
        observations=[observation],
        services_with_meta=services_with_meta,
        enrich_keys_raw=cli_args.enrich_keys,
        enrich_values_raw=cli_args.enrich_values,
        replace_existing_fields=cli_args.replace_existing_fields,
        input_table=cli_args.enrich_table_path,
        threads=cli_args.threads,
        yt_pool=cli_args.yt_pool,
        yt_config=yt_config,
    )


def parse_args():
    parser = argparse.ArgumentParser(description="Add new column to squeeze tables for one experiment")
    SqueezeEnricher.add_cli_args(parser)
    session_yt.squeeze_enricher.add_enrich_squeeze_arguments(parser)
    mstand_uargs.add_dates(parser)
    mstand_uargs.add_one_testid(parser)
    mstand_uargs.add_one_observation_id(parser)
    mstand_uargs.add_ab_token_file(parser)
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    enrich_squeeze_single(cli_args)


if __name__ == "__main__":
    main()
