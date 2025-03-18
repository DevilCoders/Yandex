#!/usr/bin/env python3

import argparse
import logging

import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqutils.nirvana_helpers as unirv


def parse_args():
    parser = argparse.ArgumentParser(description="Validate YT offline metric calculations (MSTAND-1753)")
    uargs.add_generic_params(parser)

    # build pool stage
    mstand_uargs.add_serpset_id(parser, required=False)

    return parser.parse_args()


def main_worker(cli_args):
    umisc.configure_logger(verbose=cli_args.verbose, quiet=cli_args.quiet)

    from pytlib import YtApi

    yt_api = YtApi.construct_with_yt_cluster(yt_cluster="hahn")
    rows = yt_api.read_table("//home/mstand-offline/dev/mstand_offline_metric_results_xxx")

    from collections import defaultdict
    simple_values = defaultdict(list)
    detailed_values = defaultdict(list)
    for index, row in enumerate(rows):
        if index % 10000 == 0:
            logging.info("index = %s", index)

        serpset_id = row["serpset-id"]
        metric = row["metric"]
        total_value = row["total-value"]
        pos_values = row["values-by-position"]
        # query = row["query"]
        if metric == "metric.yt-calc-test-detailed":
            assert pos_values is not None
            detailed_values[serpset_id].append(total_value)
        elif metric == "metric.yt-calc-test-simple":
            assert pos_values is None
            simple_values[serpset_id].append(total_value)
        else:
            raise Exception("Bad metric: {}".format(metric))

    import yaqutils.math_helpers as umath
    for serpset_id, values in simple_values.items():
        ss_value = umath.avg(values)
        logging.info("simple: serpset %s, value: %.10f", serpset_id, ss_value)


    import yaqutils.math_helpers as umath
    for serpset_id, values in detailed_values.items():
        ss_value = umath.avg(values)
        logging.info("detailed: serpset %s, value: %.10f", serpset_id, ss_value)

def main():
    cli_args = parse_args()
    unirv.run_and_save_exception(main_worker, cli_args)


if __name__ == "__main__":
    main()
