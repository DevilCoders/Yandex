#!/usr/bin/env python3

import argparse

import experiment_pool.pool_helpers as upool

import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc

from experiment_pool import MetricColoring
from experiment_pool import MetricValueType
from experiment_pool.metric_result import PluginABInfo


def check_unique_metric(pool):
    """
    :type pool: Pool
    :rtype:
    """
    all_metric_keys = set()
    for observation in pool.observations:
        for experiment in observation.all_experiments():
            metric_keys = experiment.get_metric_results_map().keys()
            all_metric_keys.update(metric_keys)

    if len(all_metric_keys) > 1:
        metrics = ", ".join(str(x) for x in all_metric_keys)
        raise Exception("Can't rename different metrics in pool: {}".format(metrics))


def change_metric_attributes(pool, new_name, new_hname, new_coloring, new_value_type):
    """
    :type pool: Pool
    :type new_name: str | None
    :type new_hname: str | None
    :type new_coloring: str | None
    :type new_value_type: str | None
    :rtype: None
    """
    for observation in pool.observations:
        for experiment in observation.all_experiments():
            for metric_result in experiment.metric_results:
                if new_name:
                    metric_result.metric_key.name = new_name
                    metric_result.metric_key.kwargs_name = ""
                if new_coloring:
                    metric_result.coloring = new_coloring
                if new_value_type:
                    metric_result.metric_values.value_type = new_value_type
                if new_hname:
                    if metric_result.ab_info is not None:
                        metric_result.ab_info.hname = new_hname
                    else:
                        metric_result.ab_info = PluginABInfo(hname=new_hname)


def parse_args():
    parser = argparse.ArgumentParser(description="mstand rename metric")
    uargs.add_verbosity(parser)
    mstand_uargs.add_input_pool(parser)
    uargs.add_output(parser)
    parser.add_argument(
        "--name",
        help="new metric name",
    )
    parser.add_argument(
        "--hname",
        help="new human readable metric name",
    )
    parser.add_argument(
        "--coloring",
        help="new metric coloring",
        choices=MetricColoring.ALL
    )
    parser.add_argument(
        "--value-type",
        help="new metric significant value type",
        choices=MetricValueType.ALL
    )
    return parser.parse_args()


def main():
    args = parse_args()
    umisc.configure_logger(args.verbose, args.quiet)
    pool = upool.load_pool(args.input_file)

    if args.name:
        check_unique_metric(pool)
    change_metric_attributes(pool, args.name, args.hname, args.coloring, args.value_type)

    upool.dump_pool(pool, args.output_file)

if __name__ == "__main__":
    main()
