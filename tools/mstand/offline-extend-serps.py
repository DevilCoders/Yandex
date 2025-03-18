#!/usr/bin/env python3

import argparse
import logging

import experiment_pool.pool_helpers as pool_helpers
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.file_helpers as ufile
import yaqutils.misc_helpers as umisc

from serp import ParsedSerpDataStorage
from serp import ExtendSettings

import serp.field_extender as sfe


def parse_args():
    parser = argparse.ArgumentParser(description="Add custom field to parsed serpsets")
    uargs.add_verbosity(parser)

    mstand_uargs.add_load_cache(parser, help_message="load cache with parsed serpsets from this file")
    mstand_uargs.add_save_cache(parser)
    mstand_uargs.add_storage_params(parser)
    mstand_uargs.add_modules(parser, required=False)

    # usage of shared Manager.dict() increases source TSV read time by 6-8 times
    uargs.add_threads(parser, default=1)

    uargs.add_input(parser, help_message="TSV file with (query-text, query-region, [url,] [pos,] field_value)")
    parser.add_argument(
        "--field-name",
        required=True,
        help="Field name (prefix in flat mode, use '-' for empty)",
    )
    uargs.add_boolean_argument(parser, "query-mode", "Use query-only matching", default=False)
    uargs.add_boolean_argument(parser, "with-position", inverted_name="no-position", default=False,
                               help_message="Add 'position' column to key or not")
    uargs.add_boolean_argument(parser, "overwrite", default=False, help_message="Overwrite existing field values")
    uargs.add_boolean_argument(parser, "flat-mode", default=False,
                               help_message="Add custom values directly to scales, field-name could be used as prefix")
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(verbose=cli_args.verbose, quiet=cli_args.quiet)

    if umisc.is_empty_value(cli_args.field_name):
        field_name = None
        logging.info("field name (prefix) is treated as empty value. ")
    else:
        field_name = cli_args.field_name

    extend_settings = ExtendSettings(query_mode=cli_args.query_mode,
                                     with_position=cli_args.with_position,
                                     overwrite=cli_args.overwrite,
                                     flat_mode=cli_args.flat_mode,
                                     field_name=field_name)

    load_cache = cli_args.load_cache

    parsed_serp_storage = ParsedSerpDataStorage.from_cli_args(cli_args)

    if load_cache:
        logging.info("Extracting parsed serps from cache %s", load_cache)
        parsed_serp_storage.unpack_from_tar(load_cache)

    pool = pool_helpers.load_pool(parsed_serp_storage.pool_path())
    extender = sfe.create_custom_extender_from_cli(cli_args)

    enrichment_file = cli_args.input_file

    sfe.serp_extend_main(pool=pool, enrichment_file=enrichment_file, extend_settings=extend_settings,
                         parsed_serp_storage=parsed_serp_storage, extender=extender,
                         threads=cli_args.threads)

    result_tar_file = cli_args.save_cache

    if result_tar_file:
        logging.info("Saving cache with updated serpsets to %s", result_tar_file)
        pool_helpers.dump_pool(pool, parsed_serp_storage.pool_path())
        ufile.tar_directory(path_to_pack=parsed_serp_storage.base_dir(),
                            tar_name=result_tar_file,
                            dir_content_only=False)


if __name__ == "__main__":
    main()
