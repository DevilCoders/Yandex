#!/usr/bin/env python3
import argparse
import logging
import traceback

from typing import Tuple
from typing import Union

import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqutils.nirvana_helpers as unirv

import abt.result_getter
import adminka.ab_cache
import adminka.ab_helpers
import experiment_pool.pool_helpers as pool_helpers
import mstand_utils.args_helpers as mstand_uargs

from experiment_pool import MetricColoring
from experiment_pool import Observation
from experiment_pool import Pool


def parse_args():
    parser = argparse.ArgumentParser(description="Get AB result")
    uargs.add_verbosity(parser)
    mstand_uargs.add_input_pool(parser)
    uargs.add_output(parser)
    uargs.add_threads(parser, default=10)
    mstand_uargs.add_ab_token_file(parser)
    abt.result_getter.add_abt_fetch_args(parser)
    parser.add_argument(
        "--set-coloring",
        help="override metric coloring",
        choices=MetricColoring.ALL,
        default=MetricColoring.NONE,
    )
    return parser.parse_args()


class Worker:
    def __init__(self, abt_getter: abt.result_getter.ResultGetter) -> None:
        self.abt_getter = abt_getter

    def __call__(self, observation: Observation) -> Tuple[Observation, Union[Observation, str], bool]:
        try:
            return observation, self.abt_getter.get_metric_result(observation), False
        except Exception as ex:
            logging.error("Exception in fetch worker: %s\n%s", ex, traceback.format_exc())
            return observation, str(ex), True


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    pool = pool_helpers.load_pool(cli_args.input_file)

    session = adminka.ab_cache.AdminkaCachedApi(auth_token=mstand_uargs.get_token(cli_args.ab_token_file))
    adminka.ab_helpers.validate_and_enrich(pool=pool, session=session, allow_bad_filters=True)

    bad_observations = [observation for observation in pool.observations if not observation.id]
    if bad_observations:
        raise Exception("Observations without ID: {}".format(bad_observations))

    bad_date_end_observations = [observation for observation in pool.observations if observation.dates.is_infinite()]
    if bad_date_end_observations:
        raise Exception("Observations with infinite date range can't calculate: {}".format(bad_date_end_observations))

    abt_getter = abt.result_getter.ResultGetter.from_cli_args(cli_args)

    logging.info("metric type: %s", abt_getter.params_builder.metric_type)
    logging.info("stat_fetcher_groups: %s", abt_getter.params_builder.stat_fetcher_groups)
    logging.info("stat_fetcher_granularity: %s", abt_getter.params_builder.stat_fetcher_granularity)
    logging.info("metrics: %s", abt_getter.params_builder.metric_names)
    logging.info("metric_groups: %s", abt_getter.params_builder.metric_groups)
    logging.info("detailed: %s", abt_getter.params_builder.detailed)

    count = len(pool.observations)
    logging.info("will get results for %d observations", count)

    observations = []
    get_result_args = [
        obs.clone(clone_experiments=True)
        for obs in pool.observations
    ]
    worker_get_result = Worker(abt_getter)
    results_iter = umisc.par_imap_unordered(worker_get_result, get_result_args, cli_args.threads, dummy=True)
    for pos, (observation, result, failed) in enumerate(results_iter):
        if failed:
            logging.error("abt fetch for observation %r failed with error:\n%s", observation, result)
            unirv.set_nirvana_error_status("abt fetch for observation %r failed with error:\n%s" % (observation, result))
            raise Exception(result)
        if result:
            umisc.log_progress("abt fetch", pos, count)
            unirv.log_nirvana_progress("abt fetch", pos, count)
            logging.info("Result is ready for observation %s", result)
            observations.append(result)

    pool = Pool(observations)
    pool_helpers.dump_pool(pool, cli_args.output_file)


if __name__ == "__main__":
    main()
