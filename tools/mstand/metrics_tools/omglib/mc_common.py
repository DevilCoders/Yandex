import argparse
import copy
import json
import logging
import os.path

import yaqutils.file_helpers as ufile
from yaqmetrics import MetricsMetricsClient
from .def_values import DefaultValues


def parse_args(default_dir="proxima"):
    parser = argparse.ArgumentParser(description="generate proxima and proxima components")
    parser.add_argument(
        "--token",
        help="Metrics auth token",
    )
    parser.add_argument(
        "--upload-to-metrics",
        action="store_true",
        help="update metrics (do not use if not sure)",
    )
    parser.add_argument(
        "--json-log",
        action="store_true",
        help="list metric names in JSON format",
    )
    parser.add_argument(
        "--upload-only-with-diff",
        action="store_true",
        help="skip metric if there is no diff",
    )
    parser.add_argument(
        "--directory",
        default=default_dir,
        help="where to save metrics (default={})".format(default_dir),
    )
    parser.add_argument(
        "--metric-equals",
        nargs="+",
        help="generate only one metric",
    )
    parser.add_argument(
        "--metric-contains",
        help="generate only metrics with substring",
    )
    return parser.parse_args()


def get_metrics_token_from_file():
    for metrics_token_file in [".metrics-token", "../.metrics-token"]:
        if ufile.is_file_or_link(metrics_token_file):
            logging.info("No token in command line, but file %s found, using it", metrics_token_file)
            metrics_token = ufile.read_token_from_file(metrics_token_file)
            return metrics_token
    return None


def save_generated(metrics, cli_args):
    dir_path = cli_args.directory
    metric_equals = cli_args.metric_equals
    metric_contains = cli_args.metric_contains
    upload_to_metrics = cli_args.upload_to_metrics
    upload_only_with_diff = cli_args.upload_only_with_diff
    json_log = cli_args.json_log
    metrics_token = cli_args.token

    if not metrics_token:
        metrics_token = get_metrics_token_from_file()

    if metric_equals is not None:
        metrics = [metric for metric in metrics if metric.name in metric_equals]
    if metric_contains is not None:
        metrics = [metric for metric in metrics if metric_contains in metric.name]

    logging.info("got %s metrics:\t\n%s", len(metrics), "\t\n".join(m.name for m in metrics))
    if json_log:
        list_str = json.dumps([m.name for m in metrics], sort_keys=True, indent=1)
        logging.info("names in JSON format:\n%s", list_str)
    mc_metrics = [metric.to_mc_format() for metric in metrics]

    if dir_path:
        logging.info("will save to dir: %s", dir_path)
        if not os.path.exists(dir_path):
            os.mkdir(dir_path)
        for metric in mc_metrics:
            save_metric(dir_path, metric)

    if upload_to_metrics:
        assert metrics_token, "add --token"
        client = MetricsMetricsClient(metrics_token)
        if upload_only_with_diff:
            len_before_filtering = len(mc_metrics)
            mc_metrics_by_name = {metric["name"]: reformat_metric(metric) for metric in client.get_all_metrics_info()}
            mc_metrics = [metric for metric in mc_metrics if metric != mc_metrics_by_name.get(metric["name"], {})]
            len_after_filtering = len(mc_metrics)
            logging.info("got %d/%d metrics with diff:\t\n%s",
                         len_after_filtering,
                         len_before_filtering,
                         "\t\n".join(m["name"] for m in mc_metrics))
            logging.info("will upload %d/%d metrics", len_after_filtering, len_before_filtering)
        else:
            logging.info("will upload %d metrics", len(mc_metrics))

        client.update_metrics(mc_metrics, allow_creation=True, preserved_fields=DefaultValues.DEF_PRESERVED_FIELDS)


def reformat_metric(metric):
    confidences = metric.get("confidences")
    if confidences:
        confidences.sort(key=lambda c: c["name"])

    configuration = metric.get("configuration")
    if configuration:
        requirements = configuration.get("requirements")
        if requirements:
            configuration["requirements"] = sorted(set(requirements))

        kwargs = configuration.get("kwargs")
        if kwargs:
            if kwargs.get("signals"):
                if isinstance(kwargs["signals"], str):
                    kwargs["signals"] = kwargs["signals"].split(",")
                kwargs["signals"] = sorted(set(kwargs["signals"]))

            if kwargs.get("raw_signals"):
                if isinstance(kwargs["raw_signals"], str):
                    kwargs["raw_signals"] = kwargs["raw_signals"].split(",")
                kwargs["raw_signals"] = sorted(set(kwargs["raw_signals"]))

    if "updated" in metric:
        del metric["updated"]

    if "updatedBy" in metric:
        del metric["updatedBy"]

    return metric


def save_metric(dir_path, metric):
    metric_name = metric["name"]
    file_name = "{}.json".format(metric_name)
    file_path = os.path.join(dir_path, file_name)
    with open(file_path, "w") as f:
        json.dump(metric, f, indent=2, sort_keys=True, separators=(",", ": "))


def save_metric_reformatted(dir_path, metric):
    metric = copy.deepcopy(metric)
    reformat_metric(metric)
    save_metric(dir_path, metric)


def metric_is_deprecated(metric):
    if metric.get("deprecated", False):
        return True
    description = metric.get("description") or ""
    return description == "DEPRECATED" or description.startswith("DEPRECATED:")
