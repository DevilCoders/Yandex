#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import logging

import experiment_pool.pool_helpers as phelp
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc

import reports.sensitivity_compare as rsc


def parse_args():
    parser = argparse.ArgumentParser(description="Compare metrics sensitivity")
    uargs.add_verbosity(parser)
    uargs.add_input(parser, help_message="pool(s) with metric results", multiple=True)
    mstand_uargs.add_min_pvalue(parser)
    mstand_uargs.add_threshold(parser, default=0.001)
    mstand_uargs.add_output_json(parser)
    mstand_uargs.add_output_tsv(parser)
    mstand_uargs.add_output_wiki(parser)
    mstand_uargs.add_output_html(parser)
    mstand_uargs.add_output_pool(parser, help_message="write merged pool JSON to this file")
    return parser.parse_args()


def main():
    args = parse_args()
    umisc.configure_logger(args.verbose, args.quiet)

    if not any([args.output_tsv, args.output_wiki, args.output_html, args.output_json]):
        raise Exception("Set at least one output file")

    pool = phelp.load_and_merge_pools(args.input_file)

    if args.output_pool:
        phelp.dump_pool(pool, args.output_pool)

    metric_key_id_map = phelp.generate_metric_key_id_map(pool)

    rows = rsc.generate_sensitivity_table(pool, metric_key_id_map, args.min_pvalue, args.threshold)

    rows = sorted(rows, key=lambda row: row["sensitivity"], reverse=True)

    metric_id_key_map = phelp.reverse_metric_id_key_map(metric_key_id_map)

    logging.info("dumping results to tsv")
    rsc.dump_sensitivity_table(args.output_tsv, rsc.write_tsv, rows, metric_id_key_map, args.min_pvalue, args.threshold)
    logging.info("dumping results to wiki")
    rsc.dump_sensitivity_table(args.output_wiki, rsc.write_wiki, rows, metric_id_key_map, args.min_pvalue, args.threshold)
    logging.info("dumping results to html")
    rsc.dump_sensitivity_table(args.output_html, rsc.write_html, rows, metric_id_key_map, args.min_pvalue, args.threshold)
    logging.info("dumping results to json")
    rsc.dump_sensitivity_table(args.output_json, rsc.write_json, rows, metric_id_key_map, args.min_pvalue, args.threshold)


if __name__ == "__main__":
    main()
