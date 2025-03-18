#!/usr/bin/env python3
# coding=utf-8

import argparse
import logging

import yaqutils.misc_helpers as umisc

from yaqmetrics import MetricsMetricsClient
from omglib import DefaultValues


def parse_args():
    parser = argparse.ArgumentParser(description="deprecate list of metrics using owner and description")
    parser.add_argument(
        "--token",
        required=True,
        help="auth token",
    )
    parser.add_argument(
        "--set-owner",
        help="set metric owner",
    )
    parser.add_argument(
        "--text",
        help="set metric description as DEPRECATED: <text>",
    )
    parser.add_argument(
        "--add-lamp",
        action="store_true",
        help="add special confidence/lamp",
    )
    parser.add_argument(
        "--metric-names",
        required=True,
        nargs="+",
        help="metric names",
    )
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger()

    token = cli_args.token
    owner = cli_args.set_owner
    if owner:
        logging.info("Owner will be changed to %s", owner)

    if cli_args.text:
        description = "DEPRECATED: " + cli_args.text
    else:
        description = "DEPRECATED"
    add_lamp = cli_args.add_lamp
    metric_names = set(cli_args.metric_names)

    client = MetricsMetricsClient(token)
    all_metrics = client.get_all_metrics_info()

    metrics = [metric for metric in all_metrics if metric["name"] in metric_names]
    for metric in metrics:
        if owner:
            metric["owner"] = owner
        if not metric.get("description"):
            metric["description"] = description
        elif not metric["description"].startswith("DEPRECATED"):
            metric["description"] = description + "\n\n" + metric["description"]

        metric["deprecated"] = True
        if add_lamp:
            confidences = metric.setdefault("confidences", [])
            if not any(conf["name"] == "deprecated" for conf in confidences):
                confidences.append({
                    "name": "deprecated",
                    "threshold": 0,
                    "condition": "SMALLER",
                    "requirements": [],
                })

    logging.info("will update %d metrics: %s", len(metrics), sorted(metric["name"] for metric in metrics))
    client.update_metrics(metrics, preserved_fields=DefaultValues.DEF_PRESERVED_FIELDS)


if __name__ == "__main__":
    main()
