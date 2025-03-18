#!/usr/bin/env python3

import argparse
import logging

import experiment_pool.pool_helpers as pool_helpers
import serp.serpset_fetcher as ssf
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.file_helpers as ufile
import yaqutils.misc_helpers as umisc
from serp import RawSerpDataStorage


def parse_args():
    parser = argparse.ArgumentParser(description="Serpset conversion utility")
    uargs.add_verbosity(parser)
    mstand_uargs.add_input_pool(parser, required=False)

    mstand_uargs.add_mc_serpsets_tar(parser, required=True)

    mstand_uargs.add_serp_unpack_threads(parser)
    mstand_uargs.add_serp_convert_threads(parser)

    mstand_uargs.add_save_cache(parser)
    mstand_uargs.add_storage_params(parser)

    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose)

    raw_serp_storage = RawSerpDataStorage.from_cli_args(cli_args)

    pool_file = cli_args.input_file
    if pool_file:
        pool = pool_helpers.load_pool(pool_file)
    else:
        pool = None

    mc_serpsets_tar = cli_args.mc_serpsets_tar

    ssf.unpack_mc_serpsets_main(pool=pool, mc_serpsets_tar=mc_serpsets_tar,
                                raw_serp_storage=raw_serp_storage, convert_threads=cli_args.convert_threads,
                                unpack_threads=cli_args.unpack_threads)

    if cli_args.save_cache:
        logging.info("Saving cache with fetched serpsets to %s", cli_args.save_cache)
        ufile.tar_directory(path_to_pack=raw_serp_storage.base_dir(),
                            tar_name=cli_args.save_cache,
                            dir_content_only=False)


if __name__ == "__main__":
    main()
