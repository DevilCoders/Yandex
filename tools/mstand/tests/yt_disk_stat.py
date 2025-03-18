# -*- coding: utf-8 -*-

import argparse
import functools
import logging
import os
import ujson
from multiprocessing.pool import ThreadPool

import yt.wrapper as yt

THREAD_POOL = ThreadPool(16)

YT_CONFIG = {
    "proxy": {
        "url": "hahn",
    },
}


def new_client():
    return yt.YtClient(config=YT_CONFIG)


def cached(cache):
    def decorator(func):
        def wrapper(*args, **kwargs):
            if os.path.isfile(cache):
                with open(cache) as f:
                    return ujson.load(f)
            result = func(*args, **kwargs)
            with open(cache, "w") as f:
                ujson.dump(result, f)
            return result

        return wrapper

    return decorator


def yt_list_mp(path):
    return yt.list(path, absolute=True, client=new_client())


def yt_list_many(paths):
    result = []
    for tables in THREAD_POOL.imap_unordered(yt_list_mp, paths, chunksize=4):
        result.extend(tables)
    return result


def iter_squeeze_tables(squeeze="//home/mstand/squeeze"):
    logging.info("will iter tables in %s", squeeze)
    services = yt_list_many([yt.ypath_join(squeeze, "testids")])
    testids = yt_list_many(services)
    for day_table in yt_list_many(testids):
        yield day_table

    history_services = yt_list_many([yt.ypath_join(squeeze, "history")])
    history_testids = yt_list_many(history_services)
    history_sources = yt_list_many(history_testids)
    for day_table in yt_list_many(history_sources):
        yield day_table


@cached("my_squeeze_tables.json")
def get_squeeze_tables():
    result = list(iter_squeeze_tables())
    logging.info("got %d tables", len(result))
    return result


def get_attribute_mp(table, attribute):
    value = yt.get_attribute(table, attribute, None, client=new_client())
    return table, value


def iter_with_attribute(tables, attribute):
    logging.info("will get %s attribute for %d tables", attribute, len(tables))
    worker = functools.partial(get_attribute_mp, attribute=attribute)
    return THREAD_POOL.imap_unordered(worker, tables, chunksize=4)


@cached("my_squeeze_uncompressed_size.json")
def get_squeeze_with_uncompressed_size():
    return dict(iter_with_attribute(get_squeeze_tables(), "uncompressed_data_size"))


@cached("my_squeeze_access.json")
def get_squeeze_with_access():
    return dict(iter_with_attribute(get_squeeze_tables(), "access_time"))


def print_size_stats():
    table_size = get_squeeze_with_uncompressed_size()
    logging.info("count: %s", len(table_size))
    all_sum = bytes_to_gb(sum(table_size.itervalues()))
    logging.info("sum: %s TB", bytes_to_tb(all_sum))
    logging.info("avg: %s GB", bytes_to_gb(all_sum) / len(table_size))


def bytes_to_gb(size):
    """
    :type size: int | float
    :rtype: float
    """
    return float(size) / (2 ** 30)


def bytes_to_tb(size):
    """
    :type size: int | float
    :rtype: float
    """
    return float(size) / (2 ** 40)


def write_access_stats(access_file_path):
    """
    :type access_file_path: str
    """
    table_access = get_squeeze_with_access()
    by_access = sorted(table_access.iteritems(), key=lambda (k, v): v)
    with open(access_file_path, "w") as f:
        for table, access in by_access:
            f.write(table)
            f.write("\t")
            f.write(access)
            f.write("\n")


def parse_args():
    parser = argparse.ArgumentParser(description="squeeze stats")
    parser.add_argument(
        "--size",
        action="store_true",
        help="print size stats",
    )
    parser.add_argument(
        "--access",
        action="store_true",
        help="write access stats",
    )
    return parser.parse_args()


def main():
    logging.basicConfig(format='[%(levelname)s:%(process)d] %(asctime)s - %(message)s', level=logging.INFO)
    cli_args = parse_args()
    if cli_args.size:
        print_size_stats()
    if cli_args.access:
        write_access_stats("access.tsv")


if __name__ == "__main__":
    main()
