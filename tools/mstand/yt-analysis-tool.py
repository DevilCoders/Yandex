#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import itertools
import json
import logging
import os
import shutil
import time
from multiprocessing.dummy import Pool

import numpy as np

import yt.wrapper as yt


YT_CONFIG = {
    "proxy": {
        "url": "hahn",
    },
}

YT_ATTRIBUTES = [
    "type",
    "resource_usage",
    "row_count",
]


def get_yt_client():
    return yt.YtClient(config=YT_CONFIG)


def get_testids_data(testid_path):
    testid_and_filter = testid_path.split("/")[-1].split(".")
    testid = testid_and_filter[0]
    filter_ = testid_and_filter[-1] if len(testid_and_filter) > 1 else ""

    tables = yt.list(
        testid_path,
        client=get_yt_client(),
        attributes=YT_ATTRIBUTES,
    )

    return [
        {
            "testid": testid,
            "filter": filter_,
            "date": str(table),
            "size": table.attributes["resource_usage"]["disk_space"],
            "chunks": table.attributes["resource_usage"]["chunk_count"],
            "row_count": table.attributes["row_count"],
        }
        for table in tables
    ]


def get_yt_folder_data(path, folder_name, threads):
    start = time.time()
    folder_path = "{}/{}".format(path, folder_name)

    logging.info("Get testids data from %s", folder_path)

    try:
        testids = yt.list(folder_path, client=get_yt_client())

    except yt.YtHttpResponseError as exc:
        logging.warning("Get testids data from %s failed: %s", folder_path, exc)
        return []

    args = ["{}/{}".format(folder_path, testid) for testid in testids]

    thread_pool = Pool(threads)
    data = thread_pool.map(get_testids_data, args)

    data = list(itertools.chain.from_iterable(data))

    logging.info("Received %d rows from %s in %0.2f sec.", len(data), folder_path, time.time() - start)

    return data


def bytes_to_tb(size):
    """
    :type size: int | float
    :rtype: float
    """
    return round(float(size) / (2 ** 40), 5)


def init_cache(cache_dir, cache_refresh):
    if cache_refresh and os.path.exists(cache_dir):
        logging.info("Clear cache dir: %s", cache_dir)
        shutil.rmtree(cache_dir)

    if not os.path.exists(cache_dir):
        logging.info("Create cache dir: %s", cache_dir)
        os.makedirs(cache_dir)


def save_to_cache(data, cache_dir, folder_name):
    cache_file_name = "{}.json".format(folder_name)
    cache_file_path = os.path.join(cache_dir, cache_file_name)

    logging.info("Save to cache %s", cache_file_path)

    if data:
        with open(cache_file_path, "w") as f:
            json.dump(data, f, indent=4)


def load_from_cache(cache_dir, folder_name):
    cache_file_name = "{}.json".format(folder_name)
    cache_file_path = os.path.join(cache_dir, cache_file_name)

    logging.info("Load from cache %s", cache_file_path)

    if not os.path.exists(cache_file_path):
        logging.warning("Cache file %s does not exist", cache_file_path)
        return []

    try:
        with open(cache_file_path) as f:
            return json.load(f)

    except IOError:
        logging.warning("Read from cache %s fail", cache_file_path, exc_info=True)
        return []


def get_raw_data(base_path, folders, cache_dir, threads):
    result = {}

    for folder_name in folders:
        data = load_from_cache(cache_dir, folder_name)

        if not data:
            data = get_yt_folder_data(base_path, folder_name, threads)
            save_to_cache(data, cache_dir, folder_name)

        result[folder_name] = data

    return result


class Info:
    def __init__(self, size=0, testids=0, chunks=0, base=None):
        self.size = size
        self.testids = testids
        self.chunks = chunks
        self.rates = []
        self.base = base

    def __sub__(self, other):
        return Info(self.size - other.size, self.testids - other.testids, self.chunks - other.chunks)

    def add(self, other):
        self.size += other.size
        self.testids += other.testids
        self.chunks += other.chunks

    @property
    def pretty_size(self):
        size_tb = bytes_to_tb(self.size)

        if self.base:
            percent = 100.0 * self.size / self.base.size
            return "{:0.2f} TB ({:0.2} %)".format(size_tb, percent)

        return "{:0.2f} TB".format(size_tb)

    @property
    def pretty_chunks(self):
        if self.base:
            percent = 100.0 * self.chunks / self.base.chunks
            return "{} ({:0.2} %)".format(self.chunks, percent)

        return self.chunks

    @property
    def rate_min(self):
        return "{:0.5f}".format(min(self.rates)) if self.rates else ""

    @property
    def rate_max(self):
        return "{:0.5f}".format(max(self.rates)) if self.rates else ""

    @property
    def rate_mean(self):
        return "{:0.5f}".format(np.mean(self.rates)) if self.rates else ""

    @property
    def kwargs(self):
        return {
            "size": self.pretty_size,
            "testids": self.testids,
            "chunks": self.pretty_chunks,
            "rate_min": self.rate_min,
            "rate_max": self.rate_max,
            "rate_mean": self.rate_mean,
        }

    def __str__(self):
        template = "size: {size}, testids: {testids}, chunks: {chunks}"

        if self.rates:
            template += ", rates: {rate_min}, {rate_mean}, {rate_max}"

        return template.format(**self.kwargs)


def run_analysis(data):
    total = Info()
    total_filtered = Info(base=total)

    info_by_folders = {}

    for folder_name, folder_data in data.items():
        total_folder = Info()
        total_folder_filtered = Info(base=total_folder)

        folder_data.sort(key=lambda x: x["testid"])

        for key, group in itertools.groupby(folder_data, key=lambda x: x["testid"]):
            group_by_testid = list(group)
            info, testid_info = get_testid_info(group_by_testid)
            total_folder.add(info)

            filters = set(row["filter"] for row in group_by_testid)

            if len(filters) > 1 and "" in filters:
                total_folder_filtered.add(info - testid_info)

                rate = get_testid_rate(group_by_testid, len(filters))
                if rate is not None:
                    total_folder_filtered.rates.append(rate)
                    if rate > 1.1:
                        logging.warning("Strange rate %0.2f value get in folder %s for testid %s",
                                        rate, folder_name, key)

        total.add(total_folder)
        total_filtered.add(total_folder_filtered)

        info_by_folders[folder_name] = (total_folder, total_folder_filtered)

        logging.info("Total by folder %s: %s", folder_name, total_folder)
        logging.info("Profit by folder %s: %s", folder_name, total_folder_filtered)

    logging.info("Total: %s", total)
    logging.info("Profit: %s", total_filtered)

    return info_by_folders


def get_testid_info(group_by_testid):
    info = Info(testids=1)
    testid_info = Info()

    best_by_date = {}

    for row in group_by_testid:
        info.size += row["size"]
        info.chunks += row["chunks"]

        date = row["date"]
        if date in best_by_date:
            value = best_by_date[date]
            best_by_date[date] = (min(value[0], row["size"]), min(value[1], row["chunks"]))
        else:
            best_by_date[date] = (row["size"], row["chunks"])

    for size, chunks in best_by_date.values():
        testid_info.size += size
        testid_info.chunks += chunks

    return info, testid_info


def get_testid_rate(group_by_testid, hash_count):
    group_by_date = {}
    for row in group_by_testid:
        if row["date"] in group_by_date:
            group_by_date[row["date"]].append(row)
        else:
            group_by_date[row["date"]] = [row]

    testid_row_count = 0
    filtered_row_count = 0

    for rows in group_by_date.values():
        if len(rows) == hash_count:
            for row in rows:
                if row["filter"] == "":
                    testid_row_count += row["row_count"]
                else:
                    filtered_row_count += row["row_count"]

    if testid_row_count == 0 or filtered_row_count == 0:
        return None

    return 1.0 * filtered_row_count / testid_row_count / (hash_count - 1)


def gen_wiki_report(info_by_folders, path_wiki):
    logging.info("Generate wiki report %s", path_wiki or "")

    row_template = "|| {folder}| {size}| {testids}| {chunks}| {rate_min}| {rate_mean}| {rate_max}||"

    result = [
        "#|",
        "**|| folder| size| testids| chunks| min rate| mean rate| max rate||**",
    ]

    total = Info()
    total_profit = Info(base=total)

    for folder_name in sorted(info_by_folders):
        info, profit = info_by_folders[folder_name]
        result.append(row_template.format(folder=folder_name, **info.kwargs))
        result.append(row_template.format(folder="", **profit.kwargs))

        total.add(info)
        total_profit.add(profit)

    result.append("**" + row_template.format(folder="Total", **total.kwargs) + "**")
    result.append("**" + row_template.format(folder="", **total_profit.kwargs) + "**")

    result.append("|#")

    report = "\n".join(result)

    try:
        with open(path_wiki or '', "w") as f:
            f.write(report)

    except IOError:
        logging.warning("Could not create file %s report:\n%s", path_wiki, report)


def str2bool(v):
    if v.lower() in ('yes', 'true', 't', 'y', '1'):
        return True
    elif v.lower() in ('no', 'false', 'f', 'n', '0'):
        return False
    else:
        raise argparse.ArgumentTypeError('Boolean value expected.')


example_text = '''example:

 python yt-analysis-tool.py --folders web video touch images
 python yt-analysis-tool.py --folders web --cache-refresh
 python yt-analysis-tool.py --folders web --path-wiki /tmp/report.txt
 python yt-analysis-tool.py --path //home/mstand/squeeze/testids --folders web'''


def parse_args():
    parser = argparse.ArgumentParser(
        description="Calculate testids statistics [MSTAND-1387]",
        epilog=example_text,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "--path",
        default="//home/mstand/squeeze/testids",
        help="YT path to squeeze folder (for example //home/mstand/squeeze/testids)",
    )
    parser.add_argument(
        "--folders",
        required=True,
        nargs="+",
        help="Squeeze folder names (for example, --folders web images video)",
    )
    parser.add_argument(
        "--cache-dir",
        default="/tmp/yt-analysis-tool-cache",
        help="dir with cache (default=/tmp/yt-analysis-tool-cache)",
    )
    parser.add_argument(
        "--threads",
        type=int,
        default=8,
        help="threads (default=8)",
    )
    parser.add_argument(
        "--cache-refresh",
        type=str2bool,
        nargs="?",
        const=True,
        default=False,
        help="refresh cache (default=False)",
    )
    parser.add_argument(
        "--path-wiki",
        default=None,
        help="Path to store wiki report (default=None)",
    )

    return parser.parse_args()


def main():
    start = time.time()
    logging.basicConfig(format="[%(levelname)s] %(asctime)s - %(message)s", level=logging.INFO)

    cli_args = parse_args()

    init_cache(cli_args.cache_dir, cli_args.cache_refresh)
    data = get_raw_data(
        cli_args.path,
        cli_args.folders,
        cli_args.cache_dir,
        cli_args.threads
    )
    info_by_folders = run_analysis(data)
    gen_wiki_report(info_by_folders, cli_args.path_wiki)

    logging.info("Total process time: %.2f sec.", time.time() - start)


if __name__ == "__main__":
    main()
