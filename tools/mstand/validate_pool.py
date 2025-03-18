#!/usr/bin/env python2.7

import argparse
import logging
import sys

import adminka.ab_cache
import adminka.pool_validation
import experiment_pool.pool_helpers as upool
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc

from mstand_enums.mstand_online_enums import ServiceEnum


def parse_args():
    parser = argparse.ArgumentParser(description="mstand convert pool")
    uargs.add_verbosity(parser)
    mstand_uargs.add_input_pool(parser)
    uargs.add_output(parser)
    mstand_uargs.add_ab_token_file(parser)
    mstand_uargs.add_list_of_online_services(parser, possible=ServiceEnum.get_all_services())

    return parser.parse_args()


def main():
    args = parse_args()
    umisc.configure_logger(args.verbose, args.quiet)
    pool = upool.load_pool(args.input_file)
    session = adminka.ab_cache.AdminkaCachedApi(auth_token=mstand_uargs.get_token(args.ab_token_file))

    result = adminka.pool_validation.validate_pool(pool, session=session, services=args.services)
    logging.info("Validation result:\n%s", result.pretty_print())
    upool.dump_pool(pool, args.output_file)

    if not result.is_ok():
        sys.exit(1)


if __name__ == "__main__":
    main()
