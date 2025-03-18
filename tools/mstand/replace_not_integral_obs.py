#!/usr/bin/env python2.7
import argparse
import logging

import adminka.filter_fetcher
import experiment_pool.pool_helpers as upool
from adminka.ab_cache import AdminkaCachedApi

import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc


def parse_args():
    parser = argparse.ArgumentParser(description="replace not integral observations to integral.")
    uargs.add_verbosity(parser)
    mstand_uargs.add_input_pool(parser)
    uargs.add_output(parser)
    return parser.parse_args()


def replace_not_integral_observations(observations):
    # remove pool_helpers' depends from requests module

    adminka_cached_api = AdminkaCachedApi()
    new_observations = []
    for obs in observations:
        if not obs.filters:
            new_observations.append(obs)
            continue
        task_info = adminka_cached_api.get_task_info_for_observation(obs.id)
        if task_info is None:
            raise Exception("No task {} in ab adminka.".format(obs.id))
        obs_integral = task_info['observation_integral'].replace("abt", "")
        if obs_integral == obs.id:
            raise Exception("Observation {} marked as integral, but contains filters.".format(obs.id))

        obs_info = adminka_cached_api.get_observation_info(obs.id)
        new_obs_info = adminka_cached_api.get_observation_info(obs_integral)

        if (not obs_info['datestart'] == new_obs_info['datestart'] or
                not obs_info['dateend'] == new_obs_info['dateend'] or
                not obs_info['testids'] == new_obs_info['testids'] or
                new_obs_info['filters']):
            logging.warning("incorrect id of integral observation %s in task info. Remove observation.", obs.id)
            continue

        logging.info("Replace observation id %s to integral observation id %s", obs.id, obs_integral)
        obs.id = obs_integral
        new_observations.append(obs)

    return new_observations


def replace_not_integral(pool):
    pool.observations = replace_not_integral_observations(pool.observations)
    return pool


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)
    pool = upool.load_pool(cli_args.input_file)
    adminka.filter_fetcher.fetch_all(pool, allow_bad_filters=True)
    pool = replace_not_integral(pool)
    upool.dump_pool(pool, cli_args.output_file)


if __name__ == "__main__":
    main()
