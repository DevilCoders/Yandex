#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import logging

import experiment_pool.pool_helpers as pool_helpers
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
from adminka.pool_fetcher import PoolFetcher


def parse_args():
    parser = argparse.ArgumentParser(description="Get pool")
    uargs.add_verbosity(parser)
    uargs.add_output(parser)
    PoolFetcher.add_cli_args(parser)
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    pool_fetcher = PoolFetcher.from_cli_args(cli_args)
    pool = pool_fetcher.fetch_pool()

    pool_helpers.dump_pool(pool, cli_args.output_file)
    pool.log_stats()


if __name__ == "__main__":
    main()
