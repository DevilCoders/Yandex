#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

import argparse
import copy
import logging

import pytlib.client_yt as client_yt
import pytlib.yt_io_helpers as yt_io
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqutils.time_helpers as utime

# noinspection PyPackageRequirements
import yt.wrapper as yt
# noinspection PyPackageRequirements
import yt.wrapper.common as yt_common

from yaqlibenums import YtOperationTypeEnum


def choose_users(row):
    if "key" in row and row["key"].endswith("0000"):
        yield row


def parse_args(parser):
    uargs.add_verbosity(parser)
    parser.add_argument(
        "--date-from",
        required=True,
        default="20160526",
        help="experiment start date (YYYYMMDD)",
    )
    parser.add_argument(
        "--date-to",
        required=True,
        default="20160610",
        help="experiment end date (YYYYMMDD)",
    )
    uargs.add_boolean_argument(
        parser,
        "--replace",
        help_message="collect again if table already exists",
        default=False,
    )
    return parser.parse_args()


YT_SPEC = yt_io.get_yt_operation_spec(max_failed_job_count=10,
                                      data_size_per_job=4 * yt_common.GB,
                                      operation_executor_types=YtOperationTypeEnum.MAP,
                                      use_porto_layer=False,
                                      use_scheduling_tag_filter=False,
                                      enable_input_table_index=True)


def collect_sessions(yt_client, day, sessions_type, replace, yt_pool=None):
    day_str = utime.format_date(day, pretty=True)

    logging.info("Collecting user_sessions for %s", day_str)

    src_path_base = yt.ypath_join("//user_sessions/pub", sessions_type, "daily", day_str)
    dst_path_base = yt.ypath_join("//home/mstand/test_data/user_sessions", sessions_type, "daily", day_str)

    src_path = yt.TablePath(yt.ypath_join(src_path_base, "clean"), sorted_by=["key", "subkey"], client=yt_client)
    dst_path = yt.TablePath(yt.ypath_join(dst_path_base, "clean"), sorted_by=["key", "subkey"], client=yt_client)

    if not replace and yt.exists(dst_path, client=yt_client):
        logging.info("Skip existing table: %s", dst_path)
        return

    yt_spec = copy.copy(YT_SPEC)
    if yt_pool is not None:
        yt_spec["pool"] = yt_pool  # MSTAND-1127

    with yt.TempTable("//tmp", "collect_{}_{}_".format(sessions_type, day_str), client=yt_client) as tmp:
        logging.info("running map")
        yt.run_map(
            choose_users,
            source_table=src_path,
            destination_table=tmp,
            client=yt_client,
            spec=yt_spec,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )
        yt.mkdir(dst_path_base, recursive=True, client=yt_client)

        yt_sort_spec = yt_io.get_yt_operation_spec(combine_chunks=True,
                                                   yt_pool=yt_pool,
                                                   use_porto_layer=False)

        yt.run_sort(
            tmp,
            dst_path,
            sort_by=["key", "subkey"],
            client=yt_client,
            spec=yt_sort_spec,
        )


def collect_yuid_testids(yt_client, day, replace, yt_pool=None):
    day_str = utime.format_date(day, pretty=False)

    logging.info("Collecting yuid_testids for %s", day_str)

    src_path_base = "//home/abt/yuid_testids"
    dst_path_base = "//home/mstand/test_data/yuid_testids"

    src_path = yt.TablePath(yt.ypath_join(src_path_base, day_str), sorted_by=["key", "subkey"], client=yt_client)
    dst_path = yt.TablePath(yt.ypath_join(dst_path_base, day_str), sorted_by=["key", "subkey"], client=yt_client)

    if not replace and yt.exists(dst_path, client=yt_client):
        logging.info("Skip existing table: %s", dst_path)
        return

    yt_spec = copy.copy(YT_SPEC)
    if yt_pool is not None:
        yt_spec["pool"] = yt_pool  # MSTAND-1127


    with yt.TempTable("//tmp", "collect_yuids_{}_".format(day_str), client=yt_client) as tmp:
        logging.info("running map")
        yt.run_map(
            choose_users,
            source_table=src_path,
            destination_table=tmp,
            client=yt_client,
            spec=yt_spec,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )
        yt.mkdir(dst_path_base, recursive=True, client=yt_client)
        yt_sort_spec = yt_io.get_yt_operation_spec(combine_chunks=True,
                                                   yt_pool=yt_pool,
                                                   use_porto_layer=False)

        yt.run_sort(
            tmp,
            dst_path,
            sort_by=["key", "subkey"],
            client=yt_client,
            spec=yt_sort_spec,
        )


def main():
    parser = argparse.ArgumentParser(description="collect small user sessions for tests")
    cli_args = parse_args(parser)
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    date_from = utime.parse_date_msk(cli_args.date_from)
    date_to = utime.parse_date_msk(cli_args.date_to)

    logging.basicConfig(level=logging.INFO)

    yt_client = client_yt.create_client(server="hahn")

    logging.info("Starting collecting test sessions")
    for day in utime.DateRange(date_from, date_to):
        collect_sessions(yt_client, day, "search", cli_args.replace)
        collect_sessions(yt_client, day, "watch_log_tskv", cli_args.replace)
        collect_yuid_testids(yt_client, day, cli_args.replace, cli_args.yt_pool)

    logging.info("Test sessions collection completed")


if __name__ == "__main__":
    main()
