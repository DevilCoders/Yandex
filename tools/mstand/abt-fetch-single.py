#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

import argparse
import logging

import abt.result_getter
import adminka.ab_observation
import experiment_pool as epool
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.json_helpers as ujson
import yaqutils.misc_helpers as umisc
import yaqutils.time_helpers as utime
import yaqab.ab_client


def parse_args():
    parser = argparse.ArgumentParser(description="Get AB result")
    uargs.add_verbosity(parser)
    uargs.add_output(parser)
    mstand_uargs.add_ab_token_file(parser)
    mstand_uargs.add_dates(parser)
    abt.result_getter.add_abt_fetch_args(parser)
    mstand_uargs.add_one_observation_id(parser)
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    observation_id = cli_args.observation_id
    date_from = utime.parse_date_msk(cli_args.date_from)
    date_to = utime.parse_date_msk(cli_args.date_to)

    getter = abt.result_getter.create_getter(
        cli_args.metric_type,
        cli_args.metric,
        force=cli_args.force,
        yuid_only=cli_args.yuid_only,
        services=cli_args.services,
        detailed=cli_args.detailed,
        custom_test=cli_args.custom_test,
        spuv2_path=cli_args.spuv2_path,
        cofe_project=cli_args.cofe_project,
        metric_groups=cli_args.metric_groups,
        metric_picker_key=cli_args.metric_picker_key,
    )

    client = yaqab.ab_client.AbClient(auth_token=mstand_uargs.get_token(cli_args.ab_token_file))
    info = client.get_observation(observation_id)
    logging.debug("observation info: %s", info)

    control_testid = info["testids"][0]
    exp_testids = info["testids"][1:]

    dates = utime.DateRange(date_from, date_to)

    control = epool.Experiment(testid=control_testid)
    experiments = [epool.Experiment(testid=testid) for testid in exp_testids]
    observation = epool.Observation(
        obs_id=observation_id,
        dates=dates,
        control=control,
        experiments=experiments,
    )
    logging.debug("observation: %s", observation)

    if not adminka.ab_observation.dates_ok(observation, info):
        raise Exception("Observation contains bad dates: {}".format(observation))
    result = getter.get_metric_result(observation, filter_hash=info["uid"], coloring=epool.MetricColoring.NONE)

    ujson.dump_to_file(result, cli_args.output_file)


if __name__ == "__main__":
    main()
