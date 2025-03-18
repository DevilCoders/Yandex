#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import logging
import traceback

import abt.result_getter
import adminka.ab_cache
import adminka.ab_observation
import adminka.pool_validation
import experiment_pool.pool_helpers as pool_helpers
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqutils.nirvana_helpers as unirv
import yaqab.ab_client  # noqa
from experiment_pool import MetricColoring
from experiment_pool import Observation
from experiment_pool import Pool


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
    abt.result_getter.add_abt_fetch_args(parser)
    parser.add_argument(
        "--set-coloring",
        help="override metric coloring",
        choices=MetricColoring.ALL
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
        return obs, abt_getter.get_metric_result(obs, filter_hash, coloring), False
    except Exception as ex:
        logging.error("Exception in fetch worker: %s\n%s", ex, traceback.format_exc())
        return obs, str(ex), True


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    pool = pool_helpers.load_pool(cli_args.input_file)

    session = adminka.ab_cache.AdminkaCachedApi(auth_token=mstand_uargs.get_token(cli_args.ab_token_file))
    adminka.pool_validation.validate_pool(pool, session).crash_on_error()

    bad_observations = [observation for observation in pool.observations if not observation.id]
    if bad_observations:
        raise Exception("Observations without ID: {}".format(bad_observations))

    bad_date_end_observations = [observation for observation in pool.observations if observation.dates.is_infinite()]
    if bad_date_end_observations:
        raise Exception("Observations with infinite date range can't calculate: {}".format(bad_date_end_observations))

    metric_names = [name.strip() for name in cli_args.metric] if cli_args.metric else []
    metric_groups = [name.strip() for name in cli_args.metric_groups] if cli_args.metric_groups else []

    logging.info("metric type: %s", cli_args.metric_type)
    logging.info("metrics: %s", metric_names)
    logging.info("metric_groups: %s", metric_groups)
    logging.info("force: %s", cli_args.force)
    logging.info("detailed: %s", cli_args.detailed)
    abt_getter = abt.result_getter.create_getter(
        cli_args.metric_type,
        metric_names,
        force=cli_args.force,
        yuid_only=cli_args.yuid_only,
        services=cli_args.services,
        detailed=cli_args.detailed,
        custom_test=cli_args.custom_test,
        spuv2_path=cli_args.spuv2_path,
        cofe_project=cli_args.cofe_project,
        metric_groups=metric_groups,
        metric_picker_key=cli_args.metric_picker_key,
    )
    count = len(pool.observations)
    logging.info("will get results for %d observations", count)

    # TODO: observations.clone() - to avoid copying of metric results (if any)
    obs_info_pairs = list(umisc.par_map(Worker(session.client), pool.observations, 4, dummy=True))
    bad_observations = [observation
                        for observation, info in obs_info_pairs
                        if info and not adminka.ab_observation.dates_ok(observation, info)]
    if bad_observations:
        raise Exception("Observation dates don't match dates in AB: {}".format(bad_observations))

    coloring = cli_args.set_coloring or MetricColoring.NONE

    observations = []
    get_result_args = [(abt_getter, coloring, obs.clone(clone_experiments=True), obs_info)
                       for obs, obs_info in obs_info_pairs]
    results_iter = umisc.par_imap_unordered(worker_get_result, get_result_args, abt_getter.max_threads, dummy=True)
    for pos, (observation, result, failed) in enumerate(results_iter):
        if failed:
            logging.error("abt fetch for observation %r failed with error:\n%s", observation, result)
            raise Exception(result)
        if result:
            umisc.log_progress("abt fetch", pos, count)
            unirv.log_nirvana_progress("abt fetch", pos, count)
            logging.info("result: %s", result)
            observations.append(result)

    pool = Pool(observations)
    pool_helpers.dump_pool(pool, cli_args.output_file)


if __name__ == "__main__":
    main()
