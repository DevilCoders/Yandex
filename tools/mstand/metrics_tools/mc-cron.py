#!/usr/bin/env python3

import argparse
import json
import logging
import os

import yaqutils.misc_helpers as umisc
from omglib import mc_common
from yaqmetrics import MetricsCronClient


class CRONS:
    KPI = "kpi"
    TEST = "test"
    VALIDATE = "validate"
    LAV = "lav"
    VALIDATE3H = "validate3h"

    IDS = {
        KPI: 102446,
        TEST: 102447,
        VALIDATE: 102448,
        LAV: 102449,
        VALIDATE3H: 103064,
    }


ROBOT_KPI = "robot-web-kpi"
ROBOT_CRONS = "robot-web-crons"

QUOTA_KPI = "search-quality-kpi"
QUOTA_CRONS = "search-quality-crons"

METRICS_IMPORT_SERPS = "metrics-experiments-import-serps"


def save_cron(dir_path, cron_name, cron_info):
    file_name = "{}.json".format(cron_name)
    file_path = os.path.join(dir_path, file_name)
    with open(file_path, "w") as f:
        json.dump(cron_info, f, indent=2, sort_keys=True, separators=(",", ": "))


def check_crons(cron_infos):
    kpi_info = cron_infos[CRONS.KPI]
    kpi_hosts = {}
    for param in kpi_info["experiment"]["parameters"]:
        if param["id"] == METRICS_IMPORT_SERPS:
            for host in param["value"]["downloadRequests"]:
                if host["hostId"] in kpi_hosts:
                    logging.warning("%s: duplicate host %s", CRONS.KPI, host["hostId"])
                else:
                    kpi_hosts[host["hostId"]] = host

    if not kpi_hosts:
        logging.warning("%s: kpi without hosts", CRONS.KPI)

    for cron_name, cron_info in cron_infos.items():
        if cron_info.get("deleted"):
            logging.warning("%s: deleted", cron_name)
        if not cron_info["cronExpression"]["enabled"]:
            logging.warning("%s: disabled", cron_name)
        if cron_info.get("id") != CRONS.IDS[cron_name]:
            logging.warning("%s: strange id=%s", cron_name, cron_info.get("id"))

        cron_exp = cron_info["experiment"]
        if cron_exp["owner"] != (ROBOT_KPI if cron_name == CRONS.KPI else ROBOT_CRONS):
            logging.warning("%s: unexpected owner %s", cron_name, cron_exp["owner"])
        if cron_exp["quota"] != (QUOTA_KPI if cron_name == CRONS.KPI else QUOTA_CRONS):
            logging.warning("%s: unexpected quota %s", cron_name, cron_exp["quota"])

        for param in cron_exp["parameters"]:
            if param["id"] == METRICS_IMPORT_SERPS:
                for host in param["value"]["downloadRequests"]:
                    host_id = host["hostId"]
                    kpi_host = kpi_hosts.get(host_id)
                    if not kpi_host:
                        logging.warning("%s: unknown host %s", cron_name, host_id)
                    else:
                        if host["grabbingOptions"].get("cgi") != kpi_host["grabbingOptions"].get("cgi"):
                            logging.warning("%s: unexpected cgi in host %s\n%s",
                                            cron_name,
                                            host_id,
                                            host["grabbingOptions"].get("cgi"))


def parse_args():
    parser = argparse.ArgumentParser(description="manage web crons")
    parser.add_argument(
        "--token",
        required=False,
        help="oauth token",
    )
    parser.add_argument(
        "--directory",
        default="crons",
        help="where to save crons (default=crons)",
    )
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger()

    metrics_token = cli_args.token
    if not metrics_token:
        metrics_token = mc_common.get_metrics_token_from_file()

    if not metrics_token:
        raise Exception("No metrics token specified (neither via command line `--token` option, nor via file `.metrics-token`.")

    dir_path = cli_args.directory

    if not os.path.exists(dir_path):
        os.mkdir(dir_path)

    client = MetricsCronClient(metrics_token)

    cron_infos = {}
    for cron_name, cron_id in CRONS.IDS.items():
        logging.info("getting %s (id=%d) from metrics", cron_name, cron_id)
        cron_info = client.get_cron_info(cron_id)
        logging.info("saving %s (id=%d) to disk", cron_name, cron_id)
        save_cron(dir_path, cron_name, cron_info)
        cron_infos[cron_name] = cron_info

    check_crons(cron_infos)


if __name__ == "__main__":
    main()
