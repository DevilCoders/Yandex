#!/usr/bin/env python3

import argparse
import json
import logging
from math import fabs

from yaqmetrics import MetricsMetricsClient
import yaqutils.misc_helpers as umisc

from omglib import DefaultValues

DEFAULT_OWNER = "robot-mstand"
DEFAULT_NAME_SUBSTR = ""
DEFAULT_URL = "svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/quality/mstand_metrics/offline_production/metrics"


def metric_match(item, metric_owner, metric_url, metric_module, metric_name_substr):
    if metric_owner and metric_owner != item["owner"]:
        return False

    configuration = item.get("configuration") or {}  # configuration can be None
    if metric_url and metric_url != configuration.get("url"):
        return False
    if metric_module and metric_module != configuration.get("module"):
        return False
    if metric_name_substr not in item["name"]:
        return False
    return True


def all_is_none(items):
    return all([item is None for item in items])


def check(cli_args):
    if cli_args.name_threshold or cli_args.change_to or cli_args.change_from:
        if cli_args.name_threshold and cli_args.change_to and cli_args.change_from:
            return
        else:
            raise ValueError("Impossible change thresholds: some arguments of "
                             "'name_threshold', 'change_to' or 'change_from' are missing")
    replace_args = [cli_args.replace_new, cli_args.replace_old, cli_args.replace_kwarg_name]
    if None in replace_args and not all_is_none(replace_args):
        raise ValueError("Incorrect replace parameters! replace_new is %s and replace_old is %s".format(
            cli_args.replace_new,
            cli_args.replace_old,
        ))
    all_args = replace_args + [cli_args.revision, cli_args.ram_usage_mb, cli_args.kwargs_max_depth]
    if all_is_none(all_args):
        raise ValueError("Nothing to change! All arguments is not set.")


def update_revision(metrics, cli_args):
    if cli_args.revision is None:
        return
    for item in metrics:
        item["configuration"]["revision"] = cli_args.revision


def update_ram_usage(metrics, cli_args):
    if cli_args.ram_usage_mb is None:
        return
    for item in metrics:
        item["configuration"]["ramUsageMb"] = cli_args.ram_usage_mb


def update_kwargs(metrics, cli_args):
    new_kwargs = {}
    if cli_args.kwargs_max_depth is not None:
        new_kwargs["max_depth"] = cli_args.kwargs_max_depth
    if not new_kwargs:
        return
    for item in metrics:
        kwargs = item["configuration"].setdefault("kwargs", {})
        kwargs.update(new_kwargs)


def replace_substr(metrics, cli_args):
    if cli_args.replace_new is None:
        return
    for item in metrics:
        item["configuration"]["kwargs"][cli_args.replace_kwarg_name] = item["configuration"]["kwargs"][
            cli_args.replace_kwarg_name].replace(cli_args.replace_old, cli_args.replace_new)


def update_threshold(metrics, cli_args):
    if cli_args.name_threshold is None:
        return
    if not (0 <= cli_args.change_from <= 1 and 0 <= cli_args.change_to <= 1):
        raise ValueError("Incorrect values {} or {} of thresholds!".format(cli_args.change_from, cli_args.change_to))
    for item in metrics:
        confidences = item["confidences"] or []
        if cli_args.change_to_no_yandex is None:
            update_confidences_simple(confidences, cli_args)
        else:
            update_confidences_no_yandex(confidences, cli_args)


def update_confidences_simple(confidences, cli_args):
    for confidence in confidences:
        threshold_matches = fabs(confidence["threshold"] - cli_args.change_from) < 1e-7
        if confidence["name"] == cli_args.name_threshold and threshold_matches:
            confidence["threshold"] = cli_args.change_to


def update_confidences_no_yandex(confidences, cli_args):
    if not (0 <= cli_args.change_to_no_yandex <= 1):
        raise ValueError("Incorrect values {} of no-yandex threshold!".format(cli_args.change_to_no_yandex))
    pos_yandex = None
    pos_no_yandex = None
    flag_yandex = True
    flag_no_yandex = True
    cur_pos = 0
    for confidence in confidences:
        threshold_matches = fabs(confidence["threshold"] - cli_args.change_from) < 1e-7
        if confidence["name"] == cli_args.name_threshold:
            if threshold_matches:
                if confidence["yandex"]:
                    pos_yandex = cur_pos
                    confidence["threshold"] = cli_args.change_to
                else:
                    pos_no_yandex = cur_pos
                    confidence["threshold"] = cli_args.change_to_no_yandex
            else:
                if confidence["yandex"]:
                    flag_yandex = False
                else:
                    flag_no_yandex = False
        cur_pos += 1
    if flag_no_yandex and pos_yandex is not None and pos_no_yandex is None:
        confidences.insert(pos_yandex + 1, confidences[pos_yandex].copy())
        confidences[pos_yandex + 1]["yandex"] = False
        confidences[pos_yandex + 1]["threshold"] = cli_args.change_to_no_yandex
    elif flag_yandex and pos_no_yandex is not None and pos_yandex is None:
        confidences.insert(pos_no_yandex, confidences[pos_no_yandex].copy())
        confidences[pos_no_yandex]["yandex"] = True
        confidences[pos_no_yandex]["threshold"] = cli_args.change_to


def parse_args():
    parser = argparse.ArgumentParser(description="set revision for all metrics on metrics")
    parser.add_argument(
        "--revision",
        type=int,
        default=None,
        help="new metric revision",
    )
    parser.add_argument(
        "--ram_usage_mb",
        type=int,
        default=None,
        help="new ramUsageMb value",
    )
    parser.add_argument(
        "--token",
        required=True,
        help="auth token",
    )
    parser.add_argument(
        "--owner",
        default=DEFAULT_OWNER,
        help="user name (default: {})".format(DEFAULT_OWNER),
    )
    parser.add_argument(
        "--name_substr",
        default=DEFAULT_NAME_SUBSTR,
        help="user name (default: {})".format(DEFAULT_NAME_SUBSTR),
    )
    parser.add_argument(
        "--module",
        help="metric module (ignore by default)",
    )
    parser.add_argument(
        "--url",
        default=DEFAULT_URL,
        help="metric url (default: {})".format(DEFAULT_URL),
    )
    parser.add_argument(
        "--replace_old",
        default=None,
        help="which substr to replace (default: {})".format(None),
    )
    parser.add_argument(
        "--replace_new",
        default=None,
        help="to what substr to replace (default: {})".format(None),
    )
    parser.add_argument(
        "--replace_kwarg_name",
        default=None,
        help="in which kwarg to replace (default: {})".format(None),
    )
    parser.add_argument(
        "--simulate",
        action="store_true",
        help="do not change anything",
    )
    parser.add_argument(
        "--name_threshold",
        help="what threshold should be changed"
    )
    parser.add_argument(
        "--change_from",
        type=float,
        help="threshold should be changed from this value"
    )
    parser.add_argument(
        "--change_to",
        type=float,
        help="threshold should be changed to this value"
    )
    parser.add_argument(
        "--change_to_no_yandex",
        type=float,
        help="if we need different thresholds to yandex and no-yandex, this threshold is for no-yandex"
    )
    parser.add_argument(
        "--kwargs_max_depth",
        help="set kwargs.max_depth argument (int)",
        type=int,
    )
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger()
    check(cli_args)

    token = cli_args.token
    client = MetricsMetricsClient(token)
    all_metrics = client.get_all_metrics_info()
    metrics = [item for item in all_metrics
               if metric_match(item, cli_args.owner, cli_args.url, cli_args.module, cli_args.name_substr)]

    logging.info("will update %s metrics:\t\n%s", len(metrics), "\t\n".join(m["name"] for m in metrics))

    update_threshold(metrics, cli_args)
    update_revision(metrics, cli_args)
    update_ram_usage(metrics, cli_args)
    replace_substr(metrics, cli_args)
    update_kwargs(metrics, cli_args)

    logging.info("will send to Metrics:\n %s", json.dumps(metrics, indent=1, sort_keys=True))

    if not cli_args.simulate:
        client.update_metrics(metrics, preserved_fields=DefaultValues.DEF_PRESERVED_FIELDS)


if __name__ == "__main__":
    main()
