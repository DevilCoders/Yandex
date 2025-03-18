#!/usr/bin/env python3

import argparse

import experiment_pool.pool_helpers as upool
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc


def parse_args():
    parser = argparse.ArgumentParser(description="mstand merge pools")
    uargs.add_verbosity(parser)
    uargs.add_output(parser)
    uargs.add_input(parser, help_message="mstand pool(s) to merge", multiple=True)
    uargs.add_boolean_argument(parser, "ignore-extra", help_message="ignore extra_data field")

    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)
    pools = (upool.load_pool(path) for path in cli_args.input_file)
    res_pool = upool.merge_pools(pools, ignore_extra=cli_args.ignore_extra)
    upool.dump_pool(res_pool, cli_args.output_file)


if __name__ == "__main__":
    main()
