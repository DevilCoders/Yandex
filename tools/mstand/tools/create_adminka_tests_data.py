#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

import argparse
import os.path
import sys

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import experiment_pool.pool_helpers as upool
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc

from adminka.ab_cache import AdminkaCachedApi
from adminka.pool_validation import validate_pool


def parse_args():
    parser = argparse.ArgumentParser(description="Create adminka tests data")

    uargs.add_verbosity(parser)
    uargs.add_input(parser, required=True, help_message="mstand pool")
    uargs.add_output(parser, required=True, help_message="tests data folder")

    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    pool = upool.load_pool(cli_args.input_file)
    session = AdminkaCachedApi()

    validate_pool(pool, session)

    if not os.path.exists(cli_args.output_file):
        os.makedirs(cli_args.output_file)

    assert os.path.isdir(cli_args.output_file), "{} is not folder!".format(cli_args.output_file)

    upool.dump_pool(pool, os.path.join(cli_args.output_file, "pool.json"))
    session.dump_cache(os.path.join(cli_args.output_file, "cache.json"))


if __name__ == "__main__":
    main()
