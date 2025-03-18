#!/usr/bin/env python3

import argparse
import logging
import os

import yaqutils.misc_helpers as umisc
from omglib import mc_common
from omglib import DefaultValues
from yaqmetrics import MetricsMetricsClient


REVISION_2020 = 7748549


def parse_args():
    parser = argparse.ArgumentParser(description="fix albinkessel revision")
    parser.add_argument(
        "--directory",
        default="albinkessel",
        help="where to save metrics (default=albinkessel)",
    )
    parser.add_argument(
        "--token",
        help="auth token",
    )
    return parser.parse_args()


def maybe_deprecated(metric):
    metric_name = metric["name"]
    if "ProximaZ" in metric_name:
        return True
    if "proximaKC" in metric_name:
        return True
    if "proximaS" in metric_name:
        return True
    if "proximaZ" in metric_name:
        return True
    if "proximaY" in metric_name:
        return True
    if "proximaX" in metric_name:
        return True
    if "proximaSPB" in metric_name:
        return True
    if "proxima-spb" in metric_name:
        return True
    if "proxima-SPB" in metric_name:
        return True
    if "proxima-5" in metric_name:
        return True
    if "proxima-test" in metric_name:
        return True

    if metric_name.startswith("_temp_"):
        return True
    if metric_name.startswith("_dev_"):
        return True

    if "2018" in metric_name:
        return True
    if "2019" in metric_name:
        return True

    requirements = metric["configuration"].get("requirements", [])
    if "COMPONENT.judgements.TOLOKA_5_3_MERGED" in requirements:
        return True
    if "COMPONENT.judgements.trash_toloka" in requirements:
        return True
    if "COMPONENT.judgements.trash_toloka_state" in requirements:
        return True
    if "COMPONENT.judgements.dbd_bt_score_dbddev_106" in requirements:
        return True
    if "COMPONENT.judgements.dbd_eval_bt_score" in requirements:
        return True
    if "COMPONENT.judgements.web_relevance_recall" in requirements:
        return True

    return False


def fix_metric(metric):
    updated = False

    metric_name = metric["name"]

    if metric.get("configuration") is None:
        metric["configuration"] = {}
    configuration = metric["configuration"]

    if configuration.get("kwargs") is None:
        configuration["kwargs"] = {}
    kwargs = configuration["kwargs"]

    revision = configuration.get("revision") or 0
    if revision < REVISION_2020:
        logging.info("will fix revision %d in %s", revision, metric_name)
        configuration["revision"] = REVISION_2020
        updated = True

    if "skip_3irr" in kwargs:
        logging.info("will remove skip_3irr in %s", metric_name)
        del kwargs["skip_3irr"]
        updated = True

    if "add_float_query_param" in kwargs:
        logging.info("will remove add_float_query_param in %s", metric_name)
        del kwargs["add_float_query_param"]
        updated = True

    if kwargs.get("judged") is None:
        logging.info("will set default judged=True in %s", metric_name)
        kwargs["judged"] = True
        updated = True

    if not kwargs.get("signals"):
        label_script = kwargs.get("label_script", "")
        custom_scripts = kwargs.get("custom_formulas", {})
        custom_ap_scripts = kwargs.get("custom_formulas_after_precompute", {})
        if "albin" in label_script \
            or any("albin" in s for s in custom_scripts.values()) \
            or any("albin" in s for s in custom_ap_scripts.values()):
            logging.info("will set default signals in %s", metric_name)
            kwargs["signals"] = ["albin_url_bundle", "albin_host_bundle", "albin_query_bundle"]
            updated = True

    return updated


def main():
    cli_args = parse_args()
    umisc.configure_logger()

    dir_path = cli_args.directory
    token = cli_args.token or ""

    client = MetricsMetricsClient(token)
    metrics = client.get_all_metrics_info()

    if not os.path.exists(dir_path):
        os.mkdir(dir_path)

    candidates_for_deprecation = []
    metrics_for_update = []

    for metric in metrics:
        configuration = metric.get("configuration") or {}
        module = configuration.get("module") or ""
        if module == "albinkessel" and not mc_common.metric_is_deprecated(metric):
            if maybe_deprecated(metric):
                candidates_for_deprecation.append(metric["name"])
            elif fix_metric(metric):
                metrics_for_update.append(metric)
                mc_common.save_metric_reformatted(dir_path, metric)

    if candidates_for_deprecation:
        candidates_for_deprecation.sort()
        logging.info("candidates for deprecation: %s", " ".join(candidates_for_deprecation))

    if metrics_for_update:
        metrics_for_update.sort(key=lambda metric: metric["name"])
        metric_names = [metric["name"] for metric in metrics_for_update]
        logging.info("will update %d metrics: %s", len(metrics_for_update), " ".join(metric_names))
        if token:
            client.update_metrics(metrics_for_update, preserved_fields=DefaultValues.DEF_PRESERVED_FIELDS)
        else:
            logging.info("cancel: there is no token")


if __name__ == "__main__":
    main()
