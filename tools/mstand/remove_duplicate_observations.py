#!/usr/bin/env python2.7
import argparse

import adminka.filter_fetcher
import experiment_pool.pool_helpers as upool

import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc


def parse_args():
    parser = argparse.ArgumentParser(description="remove duplicate observations from pool")
    uargs.add_verbosity(parser)
    mstand_uargs.add_input_pool(parser)
    uargs.add_output(parser)
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)
    pool = upool.load_pool(cli_args.input_file)
    adminka.filter_fetcher.fetch_all(pool, allow_bad_filters=True)
    pool = upool.remove_duplicates(pool)
    upool.dump_pool(pool, cli_args.output_file)


if __name__ == "__main__":
    main()
