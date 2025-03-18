#!/usr/bin/env python2.7
# coding=utf-8
import argparse
import logging

import experiment_pool.pool_helpers as pool_helpers
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.json_helpers as ujson
import yaqutils.misc_helpers as umisc


def parse_args():
    parser = argparse.ArgumentParser(description="Attach sbs metrics to mstand pool")
    uargs.add_verbosity(parser)

    mstand_uargs.add_input_pool(parser)
    parser.add_argument("-m", "--sbs-metric-results", required=True, help="json with sbs metric results")

    mstand_uargs.add_output_pool(parser)

    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger()

    logging.info("Start attach-sbs-metrics-to-pool script")

    pool = pool_helpers.load_pool(cli_args.input_file)
    sbs_metric_results = ujson.load_from_file(cli_args.sbs_metric_results)

    pool.fill_sbs_workflow_ids()
    pool.attach_sbs_metric_results(sbs_metric_results)
    pool_helpers.dump_pool(pool, cli_args.output_pool)

    logging.info("Finish attach-sbs-metrics-to-pool script")


if __name__ == "__main__":
    main()
