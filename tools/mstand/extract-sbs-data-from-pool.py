#!/usr/bin/env python3

import argparse
import logging

import experiment_pool.pool_helpers as pool_helpers
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.json_helpers as ujson
import yaqutils.misc_helpers as umisc


def parse_args():
    parser = argparse.ArgumentParser(description="Extract sbs data from pool for sbs-yt-read operation")
    uargs.add_verbosity(parser)

    mstand_uargs.add_input_pool(parser)
    mstand_uargs.add_output_pool(parser)

    uargs.add_output(parser, help_message="json with (sbs ticket, workflow id) pairs for further reading from yt")

    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger()

    logging.info("Start extract-sbs-data-from-pool script")

    pool = pool_helpers.load_pool(cli_args.input_file)
    pool.fill_sbs_workflow_ids()
    pool_helpers.dump_pool(pool, cli_args.output_pool)

    output_json = pool.get_sbs_ticket_workflow_pairs()
    ujson.dump_to_file(output_json, cli_args.output_file)

    logging.info("Finish extract-sbs-data-from-pool script")


if __name__ == "__main__":
    main()
