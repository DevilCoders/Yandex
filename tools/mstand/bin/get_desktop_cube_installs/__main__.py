#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import logging

from nile.api.v1 import (
    clusters,
)
from qb2.api.v1 import (
    filters as qf,
)

from qb2_extensions.api.v1.extractors import extract_desktop_distribution_tree_v2
from qb2_extensions.api.v1.extractors.pool.desktop_distribution_tree import TREE_TYPE_WITH_CAMPAIGNS

import experiment_pool.pool_helpers as upool
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc


def get_date_range(pool_path):
    pool = upool.load_pool(pool_path)
    assert len(pool.observations) == 1, "Use only pool with 1 observation"
    return [day.strftime("%Y-%m-%d") for day in pool.observations[0].dates]


def run_desktop_cube(cluster, dates, dst_table):
    logging.info("Run desktop cube processing, dst_table: %s.", dst_table)

    job = cluster.job()

    job.cumulative_table("//statbox/cube/data/desktop_install", reference_date=dates[-1]).qb2(
        log='desktop-installs-cube',
        fields=[
            'date',
            'yasoft',
            'yandexuid',
            'distr_yandexuid',
            extract_desktop_distribution_tree_v2(
                'distr_path',
                tree_type=TREE_TYPE_WITH_CAMPAIGNS,
            )
        ],
        filters=[
            qf.one_of('date', dates),
            qf.or_(qf.defined("distr_yandexuid"), qf.defined("yandexuid")),
        ]
    ).put(dst_table)

    job.run()
    logging.info("Desktop cube processing is complete.")


def parse_args():
    parser = argparse.ArgumentParser(description="Get desktop cube installs")

    uargs.add_verbosity(parser)
    uargs.add_input(parser, help_message="mstand pool")
    uargs.add_output(parser, required=True, help_message="YT path for cube installs")
    mstand_uargs.add_yt_pool(parser)

    return parser.parse_args()


def main():
    args = parse_args()
    umisc.configure_logger(args.verbose, args.quiet)

    cluster = clusters.yt.Hahn(pool=args.yt_pool).env(
        parallel_operations_limit=10,
        default_memory_limit=2048,
    )
    dates = get_date_range(args.input_file)

    run_desktop_cube(cluster, dates, args.output_file)


if __name__ == "__main__":
    main()
