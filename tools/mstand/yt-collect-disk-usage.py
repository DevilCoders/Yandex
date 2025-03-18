#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import argparse
import logging
import math
import os.path
import time

import pytlib.raw_yt_operations as raw_yt_ops
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqutils.time_helpers as utime
import yt.wrapper as yt

import mstand_utils.args_helpers as mstand_uargs
import mstand_utils.client_yt as client_yt


YT_LOG_SCHEMA = [
    {"name": "ts", "type": "uint64", "required": True},
    {"name": "ts_msk", "type": "string", "required": True},
    {"name": "path", "type": "string", "required": True},
    {"name": "folder_name", "type": "string", "required": True},
    {"name": "chunk_count", "type": "uint32", "required": True},
    {"name": "disk_space", "type": "uint64", "required": True},
    {"name": "disk_space_gb", "type": "double", "required": True},
    {"name": "table_count", "type": "uint32", "required": True},
]


class YtLogger:
    def __init__(self, yt_log_path: str, yt_client: yt.YtClient):
        self.yt_log_path = yt_log_path
        self.yt_client = yt_client

    def write(
        self,
        path: str,
        folder_name: str,
        disk_space: int,
        chunk_count: int,
        table_count: int,
    ):
        ts = time.time()
        rows = [
            dict(
                ts=int(ts),
                ts_msk=utime.timestamp_to_iso_8601(ts),
                path=path,
                folder_name=folder_name,
                disk_space=disk_space,
                chunk_count=chunk_count,
                disk_space_gb=round(disk_space / yt.common.GB, 2),
                table_count=table_count,
            )
        ]
        if not raw_yt_ops.yt_exists(self.yt_log_path, yt_client=self.yt_client):
            logging.info("Create log table: %s", self.yt_log_path)
            raw_yt_ops.yt_create_table(
                self.yt_log_path,
                ignore_existing=False,
                attributes={"schema": YT_LOG_SCHEMA},
                yt_client=self.yt_client,
            )
        raw_yt_ops.yt_append_to_table(self.yt_log_path, rows, yt_client=self.yt_client)


def pretty_size(size_bytes: int) -> str:
    if size_bytes == 0:
        return "0B"
    size_name = ("B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB")
    i = int(math.floor(math.log(abs(size_bytes), 1024)))
    p = math.pow(1024, i)
    s = round(size_bytes / p, 2)
    return "%s %s" % (s, size_name[i])


def get_folder_statistic(folder_path: str) -> (int, int):
    disk_space = chunk_count = table_count = 0
    for path in yt.search(folder_path, node_type=["table"], attributes=["resource_usage"]):
        disk_space += path.attributes["resource_usage"]["disk_space"]
        chunk_count += path.attributes["resource_usage"]["chunk_count"]
        table_count += 1
    return disk_space, chunk_count, table_count


def collect(path: str, yt_logger: YtLogger):
    logging.info("Target path: %s", path)
    folder_names = yt.list(path)
    logging.info("Got %d folders", len(folder_names))

    for name in folder_names:
        logging.info("Process %s folder", name)
        disk_space, chunk_count, table_count = get_folder_statistic(os.path.join(path, name))
        logging.info("Folder statistic: %s, %i chunks", pretty_size(disk_space), chunk_count)
        yt_logger.write(
            path=path,
            folder_name=name,
            disk_space=disk_space,
            chunk_count=chunk_count,
            table_count=table_count,
        )

    logging.info("Everything is ready")


def parse_args():
    parser = argparse.ArgumentParser(description="Collect disk usage statistic")
    uargs.add_verbosity(parser)
    mstand_uargs.add_yt(parser)

    parser.add_argument(
        "--yt-target-path",
        help="YT path to folder",
        default="//home/mstand/squeeze/testids",
    )
    parser.add_argument(
        "--yt-log-path",
        help="YT path to log table",
        default="//home/mstand/logs/disk_usage_statistic",
    )

    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    yt_config = client_yt.create_yt_config_from_cli_args(cli_args)
    yt_client = yt.YtClient(config=yt_config)
    yt_logger = YtLogger(cli_args.yt_log_path, yt_client)
    logging.info("Log path: %s", yt_logger.yt_log_path)

    collect(cli_args.yt_target_path, yt_logger)


if __name__ == "__main__":
    main()
