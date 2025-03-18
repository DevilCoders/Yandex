#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

import argparse
import logging
import traceback

import adminka.ab_observation
import adminka.ab_cache
import adminka.pool_validation
import experiment_pool.pool_helpers as pool_helpers
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.json_helpers as ujson
import yaqutils.misc_helpers as umisc
import yaqutils.time_helpers as utime
import yaqab.ab_client
from experiment_pool import Observation


class Worker:
    def __init__(self, client):
        """
        :type client: yaqab.ab_client.AbClient
        """
        self.client = client

    def __call__(self, obs):
        """
        :type obs: Observation
        :rtype: (Observation, dict | None)
        """
        try:
            if not obs.id:
                return obs, None
            return obs, self.client.get_observation(obs.id)
        except Exception as ex:
            logging.error("Exception in info worker: %s\n%s", ex, traceback.format_exc())
            raise ex


def parse_args():
    parser = argparse.ArgumentParser(description="Get AB result")
    uargs.add_verbosity(parser)
    mstand_uargs.add_input_pool(parser)
    uargs.add_output(parser)
    mstand_uargs.add_ab_token_file(parser)
    mstand_uargs.add_list_of_services(
        parser,
        default="www.yandex",
        help_message="service (default=www.yandex)",
    )
    return parser.parse_args()


def worker_get_result(arg):
    """
    :type arg: tuple[abt.getter.ResultGetter, str, Observation, dict]
    :return: tuple[Observation, dict]
    """
    abt_getter, coloring, obs, obs_info = arg
    try:
        filter_hash = obs_info["uid"] if obs_info else None
        return obs, abt_getter.get_metric_result(obs, filter_hash, coloring)
    except Exception as ex:
        logging.error("Exception in fetch worker: %s\n%s", ex, traceback.format_exc())
        raise ex


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    pool = pool_helpers.load_pool(cli_args.input_file)

    session = adminka.ab_cache.AdminkaCachedApi(auth_token=mstand_uargs.get_token(cli_args.ab_token_file))
    adminka.pool_validation.validate_pool(pool, session).crash_on_error()

    bad_observations = [observation for observation in pool.observations if not observation.id]
    if bad_observations:
        raise Exception("Observations without ID: {}".format(bad_observations))

    obs_info_pairs = umisc.par_map(Worker(session.client), pool.observations, 4, dummy=True)
    bad_observations = [observation
                        for observation, info in obs_info_pairs
                        if info and not adminka.ab_observation.dates_ok(observation, info)]
    if bad_observations:
        raise Exception("Observation dates don't match dates in AB: {}".format(bad_observations))

    result = []
    for obs, obs_info in obs_info_pairs:
        filter_hash = obs_info["uid"] if obs_info else None
        for exp in obs.all_experiments(include_control=False):
            spu_test_entry = {
                "name": obs.id,
                "dateStart": utime.format_date(obs.dates.start),
                "dateEnd": utime.format_date(obs.dates.end),
                "sideA": obs.control.testid,
                "sideB": exp.testid,
                "services": cli_args.services,
                "sessionGranularity": 30,
                "clicks": True,
            }
            if filter_hash:
                spu_test_entry["filters"] = [filter_hash]
            result.append(spu_test_entry)

    ujson.dump_json_pretty_via_native_json(result, path=cli_args.output_file)


if __name__ == "__main__":
    main()
