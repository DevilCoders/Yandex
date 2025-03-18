#!/usr/bin/env python3

import argparse
import os

import yaqutils.misc_helpers as umisc

from omglib import mc_common
from yaqmetrics import MetricsMetricsClient


def parse_args():
    parser = argparse.ArgumentParser(description="download proximas to json files")
    parser.add_argument(
        "--directory",
        default="proxima",
        help="where to save metrics (default=proxima)",
    )
    parser.add_argument(
        "--owner",
        default="robot-proxima",
        help="find metrics from this user (default=robot-proxima)",
    )
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger()

    dir_path = cli_args.directory
    find_owner = cli_args.owner

    client = MetricsMetricsClient()
    metrics = client.get_all_metrics_info()

    if not os.path.exists(dir_path):
        os.mkdir(dir_path)

    for metric in metrics:
        metric_owner = metric["owner"]
        if metric_owner == find_owner and not mc_common.metric_is_deprecated(metric):
            mc_common.save_metric_reformatted(dir_path, metric)


if __name__ == "__main__":
    main()
