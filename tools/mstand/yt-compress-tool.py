#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import collections
import datetime as dt
import logging
import math
import os.path
import time
import uuid

from multiprocessing.dummy import Pool

import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqutils.nirvana_helpers as unirv
import yaqutils.time_helpers as utime
import yt.wrapper as yt

import mstand_utils.args_helpers as mstand_uargs
import mstand_utils.client_yt as client_yt


YT_ATTRIBUTES = [
    "resource_usage",
    "uncompressed_data_size",
    "compressed_data_size",
    "compression_codec",
    "erasure_codec",
    "merge_date",
    "creation_time",
]

LOCK_TIMEOUT = 24 * 60 * 60 * 1000  # 1 day


class Table:
    def __init__(self, path, chunk_count, disk_space, uncompressed_data_size, compressed_data_size,
                 compression_codec, erasure_codec):
        """
        :type path: str
        :type chunk_count: int
        :type disk_space: int
        :type uncompressed_data_size: int
        :type compressed_data_size: int
        :type compression_codec: str
        :type erasure_codec: str
        """
        self.path = path
        self.chunk_count = chunk_count
        self.disk_space = disk_space
        self.uncompressed_data_size = uncompressed_data_size
        self.compressed_data_size = compressed_data_size
        self.compression_codec = compression_codec
        self.erasure_codec = erasure_codec
        self.ts = int(time.time())

    @staticmethod
    def from_attrs(path, attrs):
        """
        :type path: str
        :type attrs: dict[str]
        :return: Table
        """
        return Table(
            path=path,
            chunk_count=attrs["resource_usage"]["chunk_count"],
            disk_space=attrs["resource_usage"]["disk_space"],
            uncompressed_data_size=attrs["uncompressed_data_size"],
            compressed_data_size=attrs["compressed_data_size"],
            compression_codec=attrs["compression_codec"],
            erasure_codec=attrs["erasure_codec"],
        )

    @staticmethod
    def from_yt(path, client):
        """
        :type path: str
        :type client: yt.YtClient
        :return: Table
        """
        attrs = list(yt.search(path, client=client, attributes=YT_ATTRIBUTES))
        return Table.from_attrs(path, attrs[0].attributes)

    def get_info(self, status, sid, duration=None):
        """
        :type status: str
        :type sid: str
        :type duration: int | None
        :return: dict[str]
        """
        return dict(
            ts=self.ts,
            path=self.path,
            status=status,
            duration=duration,
            chunk_count=self.chunk_count,
            disk_space=self.disk_space,
            uncompressed_data_size=self.uncompressed_data_size,
            compressed_data_size=self.compressed_data_size,
            compression_codec=self.compression_codec,
            erasure_codec=self.erasure_codec,
            sid=sid,
        )

    @property
    def data_size_per_chunk_mb(self):
        """
        :return: float
        """
        if self.chunk_count == 0:
            return 0.0
        return 1.0 * self.compressed_data_size / self.chunk_count / yt.common.MB

    @property
    def rate(self):
        """
        :return: float
        """
        return self.chunk_count - self.compressed_data_size / 512. / yt.common.MB

    def __str__(self):
        return "{path}, chunks={chunk_count}".format(**self.__dict__)


class RollbackException(Exception):
    pass


class CompressResult:
    def __init__(self, old_table, new_table, duration=None, status=True, reason=None):
        """
        :type old_table: Table
        :type new_table: Table | None
        :type duration: int | None
        :type status: bool
        """
        self.old_table = old_table
        self.new_table = new_table
        self.duration = duration
        self.status = status
        self.reason = reason


class Compressor:
    def __init__(self, yt_config, compression_codec, erasure_codec, compress_all):
        """
        :type yt_config: dict[str]
        :type compression_codec: str
        :type erasure_codec: str
        :type compress_all: bool
        """
        self.yt_config = yt_config
        self.compression_codec = compression_codec
        self.erasure_codec = erasure_codec
        self.compress_all = compress_all

    def __call__(self, table):
        """
        :type table: Table
        :return: CompressResult
        """
        start = time.time()

        logging.info("Start compressing the table %s", table)
        client = yt.YtClient(config=self.yt_config)
        if not yt.exists(table.path, client=client):
            logging.warning("The table %s no longer exists", table.path)
            return CompressResult(table, None)
        with yt.Transaction(client=client) as t:
            logging.info("Wait lock of @merge_date for table %s, transaction_id: %s", table.path, t.transaction_id)
            try:
                yt.lock(
                    path=table.path,
                    mode="shared",
                    waitable=True,
                    wait_for=LOCK_TIMEOUT,
                    attribute_key="merge_date",
                    client=client,
                )
            except Exception as exc:
                logging.warning("Couldn't get lock for table %s: %s", table.path, exc, exc_info=True)
                return CompressResult(table, None, None, False, reason="no_lock")

            merge_date = yt.get_attribute(table.path, "merge_date", None, client=client)
            logging.info("Got lock of @merge_date=%s for table %s", merge_date, table.path)

            if merge_date is not None and not self.compress_all:
                logging.info("Table %s was compressed at %s.", table.path, merge_date)
                return None

            try:
                with yt.Transaction(client=client):
                    yt.transform(
                        source_table=table.path,
                        compression_codec=self.compression_codec,
                        erasure_codec=self.erasure_codec,
                        client=client,
                    )
                    new_table = Table.from_yt(table.path, client)
                    if new_table.chunk_count >= table.chunk_count:
                        raise RollbackException(
                            "Transaction was rollback: old chunks = {}, new chunks = {}".format(
                                table.chunk_count,
                                new_table.chunk_count
                            )
                        )

            except RollbackException as exc:
                logging.warning("Transaction was aborted: %s", exc, exc_info=True)
                self.set_merge_info(old_table=table, new_table=new_table, client=client)
                return CompressResult(table, new_table, None, False, reason="rollback")

            except Exception as exc:
                logging.warning("Compressing was fail: %s", exc, exc_info=True)
                return CompressResult(table, None, None, False, reason="unknown")

            self.set_merge_info(old_table=table, new_table=new_table, client=client)

        duration = int(time.time() - start)
        logging.info("Finish compressing the table %s in %s", table.path, dt.timedelta(seconds=duration))

        return CompressResult(table, new_table, duration, True)

    @staticmethod
    def set_merge_info(old_table, new_table, client):
        """
        :type old_table: Table
        :type new_table: Table
        :type client: yt.YtClient
        """
        merge_chunks_profit = max(0, old_table.chunk_count - new_table.chunk_count)
        yt.set_attribute(new_table.path, "merge_date", dt.datetime.now().isoformat(), client=client)
        yt.set_attribute(new_table.path, "merge_chunks_profit", merge_chunks_profit, client=client)


def calculate_stat(tables):
    """
    :type tables: list[Table]
    :return: tuple(int, int)
    """
    tot_chunk_count = tot_disk_space = 0
    for table in tables:
        tot_chunk_count += table.chunk_count
        tot_disk_space += table.disk_space
    return tot_chunk_count, tot_disk_space


def pretty_size(size_bytes):
    if size_bytes == 0:
        return "0B"
    size_name = ("B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB")
    i = int(math.floor(math.log(abs(size_bytes), 1024)))
    p = math.pow(1024, i)
    s = round(size_bytes / p, 2)
    return "%s %s" % (s, size_name[i])


DEFAULT_COMPRESSION_CODEC = "brotli_5"
DEFAULT_ERASURE_CODEC = "lrc_12_2_2"

YT_LOG_PATH = "//home/mstand/ivankun/compress_tables_log"
YT_LOG_SCHEMA = [
    {"name": "ts", "type": "uint32", "required": True},
    {"name": "path", "type": "string", "required": True},
    {"name": "status", "type": "string", "required": True},
    {"name": "duration", "type": "uint32", "required": False},
    {"name": "chunk_count", "type": "uint32", "required": True},
    {"name": "disk_space", "type": "uint64", "required": True},
    {"name": "uncompressed_data_size", "type": "uint64", "required": True},
    {"name": "compressed_data_size", "type": "uint64", "required": True},
    {"name": "compression_codec", "type": "string", "required": True},
    {"name": "erasure_codec", "type": "string", "required": True},
    {"name": "sid", "type": "string", "required": True},
]


class YtLogging:
    def __init__(self, path, schema, yt_config):
        """
        :type path: str
        :type schema: list[dict[str]]
        :type yt_config: dict[str]
        """
        self._path = path
        self._schema = schema
        self.client = yt.YtClient(config=yt_config)
        self._write_path = yt.TablePath(path, append=True, client=self.client)
        self._check_init = False

    def init(self):
        if not yt.exists(self._path, client=self.client):
            logging.info("Create log table: %s", self._path)
            yt.create("table", self._path, attributes={"schema": self._schema}, client=self.client)
        else:
            logging.info("Log table already exists: %s", self._path)
        self._check_init = True

    def write(self, data):
        """
        :type data: dict[str] | list[dict[str]]
        """
        if not self._check_init:
            self.init()
        self._try_transform()
        if isinstance(data, dict):
            data = [data]
        if len(data) <= 2:
            logging.info("Write to yt log:\n%r", data)
        else:
            logging.info("Write to yt log %d rows", len(data))
        yt.write_table(self._write_path, data, client=self.client)

    def _try_transform(self):
        chunk_count = yt.get_attribute(self._path, "chunk_count", client=self.client)
        if chunk_count > 500:
            logging.info("Run compression table: %s, chunk count: %d", self._path, chunk_count)
            yt.transform(self._path, client=self.client)


def get_tables_to_compress(path, yt_config, skip_compressed, limit=None, blacklist=None, creation_time_limit=None):
    """
    :type path: str
    :type yt_config: dict[str]
    :type skip_compressed: bool
    :type limit: int | None
    :type blacklist: list[str] | None
    :type creation_time_limit: datetime | None
    :return: list[Table]
    """
    def _object_filter(x):
        merge_date = x.attributes.get("merge_date")
        chunk_count = x.attributes["resource_usage"]["chunk_count"]
        creation_time = x.attributes["creation_time"]
        res = chunk_count > 1
        if skip_compressed:
            res &= merge_date is None
        if creation_time_limit is not None:
            res &= utime.iso_8601_to_datetime(creation_time) < creation_time_limit
        return res

    def _path_filter(x):
        if not blacklist:
            return True
        for folder_name in blacklist:
            if x.startswith(os.path.join(path, folder_name)):
                return False
        return True

    result = yt.search(
        path,
        node_type=["table"],
        attributes=YT_ATTRIBUTES,
        client=yt.YtClient(config=yt_config),
        object_filter=_object_filter,
        path_filter=_path_filter,
    )
    tables_to_compress = [
        Table.from_attrs(str(path), path.attributes)
        for path in result
    ]
    tables_to_compress.sort(key=lambda x: -x.rate)
    if limit is not None:
        return tables_to_compress[:limit]
    return tables_to_compress


def run_compression(
    tables_to_compress,
    compress_threads,
    yt_config,
    yt_logger,
    compression_codec,
    erasure_codec,
    compress_all,
):
    """
    :type tables_to_compress: list[Table]
    :type compress_threads:  int
    :type yt_config: dict[str]
    :type yt_logger: YtLogging
    :type compression_codec: str
    :type erasure_codec: str
    :type compress_all: bool
    """
    before_chunk_count, before_disk_space = calculate_stat(tables_to_compress)
    logging.info(
        "Total before:\n\ttables count: %d\n\t%s disk space\n\t%d chunks",
        len(tables_to_compress),
        pretty_size(before_disk_space),
        before_chunk_count,
    )

    thread_pool = Pool(compress_threads)
    yt_compress_table = Compressor(yt_config, compression_codec, erasure_codec, compress_all)
    result = thread_pool.imap_unordered(yt_compress_table, tables_to_compress)
    new_tables = []
    fall_reasons = []
    for i, comp_result in enumerate(result):
        logging.info("progress %d from %d", i + 1, len(tables_to_compress))
        unirv.log_nirvana_progress("Compression tables", i, len(tables_to_compress))

        if comp_result is None:
            continue

        # It needs to mark a couple of log rows
        sid = uuid.uuid4().hex
        yt_logger.write(comp_result.old_table.get_info("before", sid))
        if comp_result.status:
            if comp_result.new_table is not None:
                new_tables.append(comp_result.new_table)
                yt_logger.write(comp_result.new_table.get_info("success", sid, comp_result.duration))
            else:
                new_tables.append(comp_result.old_table)
                yt_logger.write(comp_result.old_table.get_info("removed", sid))
        else:
            new_tables.append(comp_result.old_table)
            fall_reasons.append(comp_result.reason)
            if comp_result.new_table:
                yt_logger.write(comp_result.new_table.get_info("worse", sid, comp_result.duration))
            else:
                yt_logger.write(comp_result.old_table.get_info("fail", sid))

    if fall_reasons:
        logging.warning("Was got the next problems:\n\t%s", collections.Counter(fall_reasons))

    after_chunk_count, after_disk_space = calculate_stat(new_tables)
    logging.info(
        "Total after:\n\ttables count: %d\n\t%s disk space\n\t%d chunks",
        len(new_tables),
        pretty_size(after_disk_space),
        after_chunk_count,
    )

    logging.info(
        "Profit:\n\t%s disk space\n\t%d chunks",
        pretty_size(before_disk_space - after_disk_space),
        before_chunk_count - after_chunk_count,
    )


def show_tables_info(title, tables):
    message = """%s
    Table count: %d
    Disk space: %s
    Compressed data size: %s
    Uncompressed data size: %s
    Chunk count: %s
    Average chunk size: %s
    """
    data_size = sum(t.compressed_data_size for t in tables)
    chunk_count = sum(t.chunk_count for t in tables)

    logging.info(
        message,
        title,
        len(tables),
        pretty_size(sum(t.disk_space for t in tables)),
        pretty_size(sum(t.compressed_data_size for t in tables)),
        pretty_size(data_size),
        chunk_count,
        pretty_size(data_size / chunk_count if chunk_count > 0 else 0),
    )


def run_analyze_quota(path, yt_config, threshold):
    """
    :type path: str
    :type yt_config: dict[str]
    :type threshold: int
    """
    result = yt.search(
        path,
        node_type=["table"],
        attributes=YT_ATTRIBUTES,
        client=yt.YtClient(config=yt_config),
    )
    tables = [Table.from_attrs(str(path), path.attributes) for path in result]
    show_tables_info("Tables with 1 chunk", [t for t in tables if t.chunk_count == 1])
    show_tables_info("All tables more 1 chunk", [t for t in tables if t.chunk_count > 1])
    show_tables_info("Tables (compressed_data_size_per_chunk > {})".format(threshold),
                     [t for t in tables if t.data_size_per_chunk_mb > threshold])
    show_tables_info("Tables (compressed_data_size_per_chunk <= {})".format(threshold),
                     [t for t in tables if t.data_size_per_chunk_mb <= threshold])
    show_tables_info("All tables", tables)


def get_timeout_limit_datetime(days_as_str):
    """
    :type days_as_str: str
    :return: dt.datetime
    """
    try:
        days = int(days_as_str)
        assert days >= 0
    except (ValueError, AssertionError):
        raise Exception("Timeout limit must be non-negative integer")
    return dt.datetime.now(tz=dt.timezone.utc) - dt.timedelta(days=days)


def parse_args():
    parser = argparse.ArgumentParser(description="Compress tables from specific directory")
    uargs.add_verbosity(parser)
    mstand_uargs.add_yt(parser)
    parser.add_argument(
        "--path",
        required=True,
        help="YT path to folder (for example //home/mstand/squeeze)",
    )
    uargs.add_boolean_argument(
        parser,
        name="--skip-compressed",
        help_message="Do not compress tables again",
        inverted_help_message="Compress all tables",
        default=True,
    )
    parser.add_argument(
        "--compress-threads",
        type=int,
        default=4,
        help="compress threads (default=4)",
    )
    parser.add_argument(
        "--yt-log-path",
        help="yt log table path (default: {})".format(YT_LOG_PATH),
        default=YT_LOG_PATH,
    )
    parser.add_argument(
        "--limit",
        type=int,
        help="Number of tables to compress (default: all)",
    )
    parser.add_argument(
        "--blacklist",
        nargs="*",
        help="List of folders that don't need to be processed",
    )
    parser.add_argument(
        "--compression-codec",
        help="Compression codec (default: {})".format(DEFAULT_COMPRESSION_CODEC),
        default=DEFAULT_COMPRESSION_CODEC,
    )
    parser.add_argument(
        "--erasure-codec",
        help="Erasure codec (default: {})".format(DEFAULT_ERASURE_CODEC),
        default=DEFAULT_ERASURE_CODEC,
    )
    uargs.add_boolean_argument(
        parser,
        name="--analyze",
        help_message="Analyze of quota usage",
        inverted_help_message="Don't analyze of quota usage",
        default=False,
    )
    parser.add_argument(
        "--threshold",
        type=int,
        help="Compressed data size per chunk, Mb (default: 512)",
        default=512,
    )
    parser.add_argument(
        "--creation-timeout-limit",
        help="compress nodes with creation_time earlier than this number of days ago",
        type=get_timeout_limit_datetime,
    )
    return parser.parse_args()


def main():
    cli_args = parse_args()

    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    yt_config = client_yt.create_yt_config_from_cli_args(cli_args)
    if cli_args.yt_pool:
        yt_config["pool"] = cli_args.yt_pool

    if cli_args.analyze:
        run_analyze_quota(cli_args.path, yt_config, cli_args.threshold)
    else:
        yt_logger = YtLogging(path=cli_args.yt_log_path, schema=YT_LOG_SCHEMA, yt_config=yt_config)

        tables_to_compress = get_tables_to_compress(
            path=cli_args.path,
            yt_config=yt_config,
            skip_compressed=cli_args.skip_compressed,
            limit=cli_args.limit,
            blacklist=cli_args.blacklist,
            creation_time_limit=cli_args.creation_timeout_limit,
        )
        logging.info("Got %d tables to compress", len(tables_to_compress))

        if tables_to_compress:
            run_compression(
                tables_to_compress=tables_to_compress,
                compress_threads=cli_args.compress_threads,
                yt_config=yt_config,
                yt_logger=yt_logger,
                compression_codec=cli_args.compression_codec,
                erasure_codec=cli_args.erasure_codec,
                compress_all=not cli_args.skip_compressed,
            )
        else:
            logging.info("Nothing to compress!")


if __name__ == "__main__":
    main()
