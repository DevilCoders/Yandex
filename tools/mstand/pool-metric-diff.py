#!/usr/bin/env python3

import argparse

import experiment_pool.pool_helpers as pool_helpers
import yaqutils.misc_helpers as umisc
from experiment_pool.filter_metric import MetricFilter


def cmp_metric_results(orig, test):
    name = orig.metric_key.name
    orig = orig.metric_values
    test = test.metric_values

    orig_values = [orig.count_val, orig.average_val, orig.sum_val, orig.value_type, orig.data_type, orig.row_count]
    test_values = [test.count_val, test.average_val, test.sum_val, test.value_type, test.data_type, test.row_count]
    if orig_values != test_values:
        return ["Values for metric {} are different:\norig: {}\ntest: {}".format(name, orig, test)]
    return []


def cmp_experiments(orig, test):
    orig_metrics = list(orig.metric_results)
    test_metrics = list(test.metric_results)
    errors = []
    for orig_metric, test_metric in zip(orig_metrics, test_metrics):
        errors.extend(cmp_metric_results(orig_metric, test_metric))
    return errors


def cmp_observations(orig, test):
    errors = []
    if orig.control:
        errors.extend(cmp_experiments(orig.control, test.control))
    if orig.experiments:
        for orig_exp, test_exp in zip(orig.experiments, test.experiments):
            errors.extend(cmp_experiments(orig_exp, test_exp))
    return errors


def cmp_pool(orig, test):
    orig_keys = orig.all_metric_keys()
    test_keys = test.all_metric_keys()
    check_metric_names = [key.name for key in orig_keys & test_keys]

    metric_filter = MetricFilter(metric_whitelist=check_metric_names)

    metric_filter.filter_metric_for_pool(orig)
    metric_filter.filter_metric_for_pool(test)

    orig.sort()
    test.sort()

    errors = []
    for orig_obs, test_obs in zip(orig.observations, test.observations):
        errors.extend(cmp_observations(orig_obs, test_obs))

    if errors:
        raise Exception("\n".join(errors))


def main():
    umisc.configure_logger(verbose=0)
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--orig",
        help="path to the original data"
    )

    parser.add_argument(
        "--test",
        help="path to the test data"
    )

    cli_args = parser.parse_args()

    orig = pool_helpers.load_pool(cli_args.orig)
    test = pool_helpers.load_pool(cli_args.test)
    cmp_pool(orig, test)


if __name__ == "__main__":
    main()
