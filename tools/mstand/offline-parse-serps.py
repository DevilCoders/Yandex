#!/usr/bin/env python3
# coding=utf-8

import argparse
import logging

import experiment_pool.pool_helpers as pool_hepers
import serp.serpset_parser as ssp
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqutils.nirvana_helpers as unirv

from serp import RawSerpDataStorage
from serp import ParsedSerpDataStorage
from serp import PoolParseContext
from serp import SerpAttrs


def parse_args():
    parser = argparse.ArgumentParser(description="Parse fetched serpsets")

    uargs.add_generic_params(parser)
    mstand_uargs.add_storage_params(parser)

    mstand_uargs.add_load_cache(parser)
    mstand_uargs.add_save_cache(parser)

    mstand_uargs.add_serp_attributes(parser)
    mstand_uargs.add_serp_parse_params(parser)

    uargs.add_threads(parser, default=2)

    return parser.parse_args()


def main_worker(cli_args):
    umisc.configure_logger(verbose=cli_args.verbose, quiet=cli_args.quiet)

    raw_serp_storage = RawSerpDataStorage.from_cli_args(cli_args)
    parsed_serp_storage = ParsedSerpDataStorage.from_cli_args(cli_args)

    if cli_args.load_cache:
        logging.info("Extracting data cache from %s", cli_args.load_cache)
        raw_serp_storage.unpack_from_tar(cli_args.load_cache)

    logging.info("Extracting pool from cache %s", raw_serp_storage.pool_path())
    pool = pool_hepers.load_pool(raw_serp_storage.pool_path())

    serp_attrs = SerpAttrs.from_cli_args(cli_args)
    pool_parse_ctx = PoolParseContext(serp_attrs=serp_attrs,
                                      raw_serp_storage=raw_serp_storage,
                                      parsed_serp_storage=parsed_serp_storage,
                                      threads=cli_args.threads,
                                      allow_no_position=cli_args.allow_no_position,
                                      allow_broken_components=cli_args.allow_broken_components,
                                      remove_raw_serpsets=cli_args.remove_raw_serpsets)

    ssp.parse_serpsets(pool, pool_parse_ctx)

    if cli_args.save_cache:
        # TODO: make separate workdirs => cleanup of raw serpsets storage below will be not necessary
        # reduce cache size (drop already processed data)
        raw_serp_storage.cleanup()

        pool_hepers.dump_pool(pool, parsed_serp_storage.pool_path())
        tar_name = cli_args.save_cache
        logging.info("Saving parsed serpsets to %s", tar_name)
        parsed_serp_storage.pack_to_tar(tar_name)


def main():
    cli_args = parse_args()
    unirv.run_and_save_exception(main_worker, cli_args)


if __name__ == "__main__":
    main()
