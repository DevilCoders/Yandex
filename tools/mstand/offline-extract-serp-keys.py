#!/usr/bin/env python3

import argparse
import logging

import experiment_pool.pool_helpers as pool_helpers
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqutils.file_helpers as ufile

from serp import ParsedSerpDataStorage
from serp import DumpSettings
import serp.keys_dumper as skd


def parse_args():
    parser = argparse.ArgumentParser(description="Print all keys (qid, region, url) in tsv format")
    uargs.add_verbosity(parser)

    mstand_uargs.add_load_cache(parser, help_message="load cache with parsed serpsets from this file")
    uargs.add_output(parser)

    mstand_uargs.add_storage_params(parser)

    uargs.add_boolean_argument(parser, "query-mode", inverted_name="url-mode",
                               default=False, help_message="Query-only mode")

    # these options have 'negative' prefix because they drop part of query key.
    # i.e. they should NOT be used in general case.

    uargs.add_boolean_argument(parser, "no-device", inverted_name="with-device",
                               default=False, help_message="Exclude 'device' column or not ")
    uargs.add_boolean_argument(parser, "no-uid", inverted_name="with-uid",
                               default=False, help_message="Exclude 'uid' column or not ")
    uargs.add_boolean_argument(parser, "no-country", inverted_name="with-country",
                               default=False, help_message="Exclude 'country' column or not")

    uargs.add_boolean_argument(parser, "with-position", inverted_name="no-position",
                               default=False, help_message="Extract 'position' column or not")
    uargs.add_boolean_argument(parser, "with-qid", inverted_name="no-qid",
                               default=False, help_message="Extract 'qid' column (internal query ID)")
    uargs.add_boolean_argument(parser, "with-serpset-id", inverted_name="no-serpset-id",
                               default=False, help_message="Extract 'serpset ID' column or not")

    parser.add_argument("--serp-depth", type=int, help="Limit SERP depth with this value")


    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose)

    with_position = cli_args.with_position
    if cli_args.query_mode:
        if with_position:
            with_position = False
            logging.warning("Query-only mode enabled, disabling 'position' column.")

    parsed_serp_storage = ParsedSerpDataStorage(root_dir=cli_args.root_cache_dir,
                                                cache_subdir=cli_args.cache_subdir,
                                                use_cache=cli_args.use_cache)

    if cli_args.load_cache:
        logging.info("Extracting parsed serps from cache %s", cli_args.load_cache)
        parsed_serp_storage.unpack_from_tar(cli_args.load_cache)

    pool = pool_helpers.load_pool(parsed_serp_storage.pool_path())

    with_device = not cli_args.no_device
    with_uid = not cli_args.no_uid
    with_country = not cli_args.no_country

    dump_settings = DumpSettings(query_mode=cli_args.query_mode,
                                 with_qid=cli_args.with_qid,
                                 with_device=with_device,
                                 with_uid=with_uid,
                                 with_country=with_country,
                                 with_position=with_position,
                                 with_serpset_id=cli_args.with_serpset_id,
                                 serp_depth=cli_args.serp_depth)

    logging.info("Keys dump settings: %s", dump_settings)

    keys = skd.get_all_keys(pool.all_serpset_ids(), parsed_serp_storage, dump_settings=dump_settings)
    ufile.save_tsv(keys, cli_args.output_file)
    logging.info("Serpset keys extraction done.")


if __name__ == "__main__":
    main()
