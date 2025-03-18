#!/usr/bin/env python3
# coding=utf-8
import argparse
import itertools

import yaqutils.json_helpers as ujson
import yaqutils.requests_helpers as urequests
from user_plugins import PluginSource, PluginBatch, PluginKwargs

DEFAULT_URL = "svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/quality/mstand_metrics/offline_production/metrics"
ASPECT_METRICS_URL = "https://metrics-admin.qloud.yandex-team.ru/api/layout/{}/{}"
ALL_METRICS_URL = "https://metrics-join.qe.yandex-team.ru/api/metric-specification/"
CONFIG_KEY = "configuration"


def parse_args():
    parser = argparse.ArgumentParser(description="export metrics batch")
    parser.add_argument("--revision", required=True, help="revision")
    parser.add_argument("--save-batch", required=True, help="batch file")
    parser.add_argument("--save-modified-batch", required=True, help="modified batch file")
    parser.add_argument("--save-requirements", required=True, help="requirements")
    parser.add_argument("--aspects", help="aspect names to process", nargs="+", default=["proxima", "proxima_spb"])
    parser.add_argument("--evaluations", help="evaluations to process", nargs="+", default=["WEB", "MOBILE"])
    return parser.parse_args()


def get_aspect_metrics(evaluations, aspect_names):
    pairs = list(itertools.product(evaluations, aspect_names))
    urls = [ASPECT_METRICS_URL.format(evaluation, name) for evaluation, name in pairs]
    aspects = [urequests.retry_request(method="GET", url=url, verify=False).json() for url in urls]

    result = set()
    for aspect in aspects:
        for group in aspect.values():
            result.update(group)

    return result


def metric_matches(metric, aspect_metrics):
    if not metric:
        return False

    config = metric.get(CONFIG_KEY) or {}
    url = config.get("url")
    return url and url.startswith(DEFAULT_URL) and metric.get("name") in aspect_metrics


def parse_kwargs(kwargs):
    return [PluginKwargs(name="default", kwargs=kwargs)]


def parse_plugin_source(metric):
    config = metric[CONFIG_KEY]
    return PluginSource(
        module_name=config.get("module"),
        class_name=config.get("className"),
        url=config.get("url"),
        revision=config.get("revision"),
        alias=metric.get("name"),
        kwargs_list=parse_kwargs(config.get("kwargs"))
    )


def parse_requirements(metric):
    config = metric[CONFIG_KEY]
    return config["requirements"]


def update_revision(plugin_source, revision):
    plugin_source.revision = revision
    return plugin_source


def main():
    cli_args = parse_args()

    metrics = urequests.retry_request(method="GET", url=ALL_METRICS_URL, verify=False).json()
    aspect_metrics = get_aspect_metrics(cli_args.evaluations, cli_args.aspects)

    matched_metrics = [m for m in metrics if metric_matches(m, aspect_metrics)]

    requirements = set()
    for metric in matched_metrics:
        requirements.update(parse_requirements(metric))

    plugin_sources = [parse_plugin_source(m) for m in matched_metrics]
    plugin_batch = PluginBatch(plugin_sources)
    ujson.dump_to_file(plugin_batch.serialize(), cli_args.save_batch)

    new_plugin_sources = [update_revision(s, cli_args.revision) for s in plugin_sources]
    new_plugin_batch = PluginBatch(new_plugin_sources)
    ujson.dump_to_file(new_plugin_batch.serialize(), cli_args.save_modified_batch)
    ujson.dump_to_file({"requirements": sorted(requirements)}, cli_args.save_requirements)


if __name__ == "__main__":
    main()
