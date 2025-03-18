#!/usr/bin/env python3

import argparse
import logging

import experiment_pool.pool_helpers as pool_helpers
import serp.serpset_fetcher as ssf
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
from serp import RawSerpDataStorage
from serp import SerpFetchParams


def parse_args():
    parser = argparse.ArgumentParser(description="Serpset fetching utility")
    uargs.add_verbosity(parser)
    # debug option: serpset filter
    mstand_uargs.add_serpset_id_filter(parser, required=False)
    mstand_uargs.add_input_pool(parser)

    mstand_uargs.add_serp_fetch_params(parser, use_external_convertor_default=False)
    mstand_uargs.add_serp_attributes(parser)

    mstand_uargs.add_save_cache(parser)
    mstand_uargs.add_storage_params(parser)

    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(verbose=cli_args.verbose, quiet=cli_args.quiet)

    raw_serp_storage = RawSerpDataStorage.from_cli_args(cli_args)
    pool = pool_helpers.load_pool(cli_args.input_file)

    fetch_params = SerpFetchParams.from_cli_args(cli_args)

    ssf.fetch_serpsets_main(pool=pool, fetch_params=fetch_params,
                            raw_serp_storage=raw_serp_storage)

    if cli_args.save_cache:
        tar_name = cli_args.save_cache
        logging.info("Saving cache with fetched serpsets to %s", tar_name)
        raw_serp_storage.pack_to_tar(tar_file_name=tar_name)


if __name__ == "__main__":
    main()
