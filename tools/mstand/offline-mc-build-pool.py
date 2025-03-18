#!/usr/bin/env python3
# coding=utf-8

import argparse
import logging

import experiment_pool.pool_helpers as phelp
import serp.serp_helpers as shelp

import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc


def parse_args():
    parser = argparse.ArgumentParser(description="Serpset pool builder")
    uargs.add_verbosity(parser)
    mstand_uargs.add_serpset_id(parser)
    uargs.add_output(parser)
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose)

    # could be one or many. Metrics currently uses one serpset only
    serpsets = cli_args.serpset_id
    logging.info("Building pool from serpsets: %s", serpsets)
    pool = shelp.build_mc_pool(serpsets=serpsets)

    logging.info("Saving pool to %s", cli_args.output_file)
    phelp.dump_pool(pool, cli_args.output_file)


if __name__ == "__main__":
    main()
