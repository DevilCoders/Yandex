#!/usr/bin/env python3

import argparse
import logging

import yaqutils.misc_helpers as umisc

from omglib import mc_common
from yaqmetrics import MetricsMetricsClient


def parse_args():
    parser = argparse.ArgumentParser(description="find mstand metrics from metrics")
    parser.add_argument(
        "--owner",
        help="owner",
    )
    parser.add_argument(
        "--module",
        default="albinkessel",
        help="module (default=albinkessel)",
    )
    parser.add_argument(
        "--class-name",
        default="AlbinKessel",
        help="className (default=AlbinKessel)",
    )
    parser.add_argument(
        "--requirements",
        nargs="*",
        help="requirements (for example COMPONENT.judgements.RELEVANCE)",
    )
    parser.add_argument(
        "--with-usage",
        help="try to detect judgement usage (for example rel)",
    )
    parser.add_argument(
        "--without-usage",
        help="try to detect judgement usage (for example rel)",
    )
    parser.add_argument(
        "--without-flag",
        help="see if some flag enabled (for example skip_3irr)",
    )
    parser.add_argument(
        "--wiki",
        action="store_true",
        help="use wiki format for output",
    )
    return parser.parse_args()


def detect_usage(metric_kwargs, substring):
    if metric_kwargs.get("label_script") and substring in metric_kwargs.get("label_script"):
        return True

    for formula in metric_kwargs.get("custom_formulas", {}).itervalues():
        if substring in formula:
            return True

    for formula in metric_kwargs.get("custom_formulas_after_precompute", {}).itervalues():
        if substring in formula:
            return True

    return False


def check_metric(metric, cli_args):
    if mc_common.metric_is_deprecated(metric):
        logging.info("skip %s: deprecated", metric["name"])
        return False

    if cli_args.owner and metric.get("owner") != cli_args.owner:
        logging.info("skip %s: wrong owner", metric["name"])
        return False

    configuration = metric.get("configuration") or {}

    if cli_args.module and configuration.get("module") != cli_args.module:
        logging.info("skip %s: wrong module", metric["name"])
        return False

    if cli_args.class_name and configuration.get("className") != cli_args.class_name:
        logging.info("skip %s: wrong class", metric["name"])
        return False

    if cli_args.requirements and set(configuration.get("requirements", [])).isdisjoint(cli_args.requirements):
        logging.info("skip %s: wrong requirements", metric["name"])
        return False

    metric_kwargs = configuration.get("kwargs", {})

    if cli_args.without_usage and detect_usage(metric_kwargs, cli_args.without_usage):
        logging.info("skip %s: should be without %s", metric["name"], cli_args.without_usage)
        return False

    if cli_args.with_usage and not detect_usage(metric_kwargs, cli_args.with_usage):
        logging.info("skip %s: should be with %s", metric["name"], cli_args.with_usage)
        return False

    if cli_args.without_flag and metric_kwargs.get(cli_args.without_flag):
        logging.info("skip %s: should be without flag %s", metric["name"], cli_args.without_flag)
        return False

    logging.info("use %s", metric["name"])
    return True


def main():
    cli_args = parse_args()
    umisc.configure_logger()

    wiki = cli_args.wiki

    client = MetricsMetricsClient()
    metrics = client.get_all_metrics_info()
    logging.info("%d metrics before filtering", len(metrics))

    metrics = [metric for metric in metrics if check_metric(metric, cli_args)]
    logging.info("%d metrics after filtering", len(metrics))

    for metric in metrics:

        if wiki:
            print("((https://metrics.yandex-team.ru/admin/all-metrics/{0} {0}))".format(metric["name"]))
        else:
            print(metric["name"])


if __name__ == "__main__":
    main()
