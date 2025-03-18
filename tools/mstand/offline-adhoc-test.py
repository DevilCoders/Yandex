#!/usr/bin/env python3

import logging
import argparse

import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqutils.nirvana_helpers as unirv

import serp.mstand_offline_adhoc as mstand_adhoc


def parse_args():
    parser = argparse.ArgumentParser(description="Test offline adhoc API for jupyter (MSTAND-1668)")
    uargs.add_generic_params(parser)

    return parser.parse_args()


def main_worker(cli_args):
    umisc.configure_logger(verbose=cli_args.verbose, quiet=cli_args.quiet)

    metric_ctx = mstand_adhoc.load_metric(class_name="FiveCGDetailed", module_name="metrics.offline.five_cg_detailed")

    serp_attrs = mstand_adhoc.make_serp_attrs(judgements=["RELEVANCE"])
    parsed_serps = mstand_adhoc.load_serpset("2.json", serp_attrs=serp_attrs)

    for parsed_serp in parsed_serps:
        smv = mstand_adhoc.compute_metric_on_serp(metric_ctx=metric_ctx, parsed_serp=parsed_serp)
        logging.info("smv details: qid = %s, total = %s, pos values: %s",
                     smv.qid, smv.get_total_value(), smv.get_values_by_position())


def main():
    cli_args = parse_args()
    unirv.run_and_save_exception(main_worker, cli_args)


if __name__ == "__main__":
    main()
