#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import logging

from nile.api.v1 import (
    clusters,
    files as nfi,
    Record,
)
from qb2.api.v1 import (
    filters as sf,
    extractors as se,
)

from statutils.common.distribution.mobile_distribution_tree_v4 import \
    extract_mobile_distribution_tree_v4 as mdt_v4

import experiment_pool.pool_helpers as upool
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc


def get_date_range(pool_path):
    pool = upool.load_pool(pool_path)
    assert len(pool.observations) == 1, "Use only pool with 1 observation"
    return [day.strftime("%Y-%m-%d") for day in pool.observations[0].dates]


def mobile_cube_reducer(groups):
    for key, recs in groups:
        info = []
        for r in recs:
            if r["distr_efforts"] is not None:
                info.extend(r["distr_efforts"])

        res = set()
        source = "hz"
        for t in info:
            if t["yandexuid_com"]:
                res.add("y" + t["yandexuid_com"])
            if t["yandexuid_ru"]:
                res.add("y" + t["yandexuid_ru"])
            if t["url_parameters"] and t["url_parameters"].get("source_id") == "serp_bk":
                source = "portal_serp"

        if len(res) == 1:
            yield Record(yuid=list(res)[0], source=source)


def run_mobile_activations_cube(cluster, dates, dst_table):
    logging.info("Run mobile cube processing, dst_table: %s.", dst_table)

    job = cluster.job()

    job \
        .cumulative_table(
            "//cubes/mobile_activations",
            reference_date=dates[-1],
        ) \
        .filter(
            sf.one_of("start_date", dates),
            sf.equals("activation_type", "activation"),
        ) \
        .project(
            "device_id",
            "distr_efforts",
            se.dictitem("clid", from_="selected_clid"),
            mdt_v4("distr_channel"),
            files=[
                nfi.StatboxDict("distr_report.json", use_latest=True),
                nfi.StatboxDict(
                    "appsflyer_to_tracking_v3.yaml",
                    use_latest=True
                ),
                nfi.StatboxDict(
                    "tracking_publisher_groups_v4.yaml",
                    use_latest=True
                ),
                nfi.StatboxDict(
                    "apps_projects_to_browser.yaml",
                    use_latest=True
                ),
                nfi.StatboxDict(
                    "retailer_to_domain.json",
                    use_latest=True
                ),
            ]
        ) \
        .filter(
            sf.contains("distr_channel", "portal"),
        ) \
        .groupby("device_id") \
        .reduce(mobile_cube_reducer, memory_limit=2048) \
        .sort("yuid") \
        .put(
            dst_table,
            schema={"yuid": str, "source": str},
        )

    job.run()
    logging.info("Mobile cube processing is complete.")


def parse_args():
    parser = argparse.ArgumentParser(description="Get portal cube installs")

    uargs.add_verbosity(parser)
    uargs.add_input(parser, help_message="mstand pool")
    uargs.add_output(parser, required=True, help_message="YT path for cube installs")

    return parser.parse_args()


def main():
    args = parse_args()
    umisc.configure_logger(args.verbose, args.quiet)

    cluster = clusters.yt.Hahn().env(
        parallel_operations_limit=10,
        default_memory_limit=2048,
    )
    dates = get_date_range(args.input_file)

    run_mobile_activations_cube(cluster, dates, args.output_file)


if __name__ == "__main__":
    main()
