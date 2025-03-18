#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse

import adminka.ab_cache
import adminka.pool_validation
import adminka.pool_fetcher as pool_fetcher
import experiment_pool.pool_helpers as pool_helpers
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqutils.time_helpers as utime


def parse_args():
    parser = argparse.ArgumentParser(description="Get single pool")
    uargs.add_verbosity(parser)
    uargs.add_output(parser)
    mstand_uargs.add_dates(parser, required=False)
    mstand_uargs.add_one_observation_id(parser)
    parser.add_argument(
        "--extra-data",
        help="get observation's metadata from AB",
        action="store_true"
    )
    uargs.add_boolean_argument(
        parser,
        "validate-testids",
        help_message="validate testids for adminka/adv testid format",
        default=True,
    )
    uargs.add_boolean_argument(
        parser,
        "split-by-day",
        help_message="Split pool by day",
        default=False,
    )
    mstand_uargs.add_ab_token_file(parser)
    return parser.parse_args()


def check_date(date_to_check, observation):
    if not date_to_check:
        return False

    if date_to_check in observation.dates:
        return True

    raise ValueError("input date {} not in [{},{}]".format(
        date_to_check, observation.dates.start, observation.dates.end
    ))


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    observation_id = cli_args.observation_id
    date_from = utime.parse_date_msk(cli_args.date_from)
    date_to = utime.parse_date_msk(cli_args.date_to)

    if date_from and date_to and date_from > date_to:
        raise Exception("date_from > date_to")

    session = adminka.ab_cache.AdminkaCachedApi(auth_token=mstand_uargs.get_token(cli_args.ab_token_file))
    pool = pool_fetcher.fetch_pool_by_observation_id(
        observation_id,
        adminka_session=session,
        extra_data=cli_args.extra_data,
        validate_testids=cli_args.validate_testids,
    )

    assert len(pool.observations) == 1

    observation = pool.observations[0]
    if not cli_args.validate_testids or check_date(date_from, observation):
        observation.dates.start = date_from
    if not cli_args.validate_testids or check_date(date_to, observation):
        observation.dates.end = date_to

    if cli_args.validate_testids:
        validation_result = adminka.pool_validation.validate_pool(pool, session)
        error_testids = validation_result.all_error_testids()[observation]
        if observation.control.testid in error_testids:
            raise ValueError(
                "control's testid validation error:\n{}"
                .format(validation_result.pretty_print())
            )
        observation.remove_testids(validation_result.all_error_testids()[observation])

    if cli_args.split_by_day and observation.dates.number_of_days() > 1:
        daily_obs = pool_helpers.get_daily_observations(observation)
        pool.observations.extend(daily_obs)

    pool_helpers.dump_pool(pool, cli_args.output_file)
    pool.log_stats()


if __name__ == "__main__":
    main()
