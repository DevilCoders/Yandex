#!/usr/bin/env python2.7

import argparse
import logging

import experiment_pool.pool_helpers as upool

import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqab.ab_client


def parse_args():
    parser = argparse.ArgumentParser(description="mstand convert pool")
    uargs.add_verbosity(parser)
    mstand_uargs.add_ab_token_file(parser)
    mstand_uargs.add_input_pool(parser)
    uargs.add_output(parser)
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)
    pool = upool.load_pool(cli_args.input_file)
    client = yaqab.ab_client.AbClient(auth_token=mstand_uargs.get_token(cli_args.ab_token_file))

    for observation in pool.observations:
        obs_info = client.get_observation(observation.id)
        tags = obs_info["tags"]
        if tags:
            logging.info("Loaded tags for observation %s: %s", observation, ", ".join(tags))
            observation.tags = tags

    upool.dump_pool(pool, cli_args.output_file)


if __name__ == "__main__":
    main()
