import yt.wrapper as yt

from collections import defaultdict
import importlib
import json
import logging
import os
import zlib

from cloud.blockstore.tools.analytics.common import add_expiration_time

tracelib = importlib.import_module(
    "cloud.blockstore.tools.analytics.find-perf-bottlenecks.lib")


#
#   Building 'representative' slow tracks
#


def log_line2track(row, tag):
    trace = tracelib.extract_minimized_trace(row, tag)
    if trace is None:
        return

    out = {
        "Request": trace.request,
        "MediaKind": trace.media_kind,
        "Track": tracelib.describe_trace(trace),
        "Times": [x.ts for x in trace.probes],
        "LogLine": row,
        "Length": trace.probes[-1].ts - trace.probes[0].ts
    }
    yield out


class Bucket:

    def __init__(self):
        self.count = 0

    def update(self, times, log):
        if self.count == 0:
            self.times = times
            self.log = zlib.compress(json.dumps(log).encode('utf-8'), 9)
        self.count += 1


class Total:

    def __init__(self):
        self.count = 0

    def update(self, times):
        if self.count == 0:
            self.times = list(times)
        else:
            for i in range(len(times)):
                self.times[i] += times[i]
        self.count += 1


def percentile(p, buckets, count):
    target = min(int(count * (p / 100.0)), count - 1)
    count = 0
    for k in sorted(buckets):
        count += buckets[k].count
        if count >= target:
            return buckets[k]


def aggregate_tracks(key, rows):
    buckets = defaultdict(Bucket)
    total = Total()

    for row in rows:
        buckets[row["Length"] // 1000].update(row["Times"], row["LogLine"])
        total.update(row["Times"])

    def track_info(p):
        bucket = percentile(p, buckets, total.count)
        return {
            "T": bucket.times,
            "L": json.loads(zlib.decompress(bucket.log)),
        }

    out = {
        "TrackCount": total.count,
        "Times": {
            "Average": {"T": [t // total.count for t in total.times]},
            "Median": track_info(50),
            "P95": track_info(95),
            "P99": track_info(99),
        }
    }

    out.update(key)
    yield out


class DescribeSlowRequestsResult(object):

    def __init__(self, dst_table):
        self.dst_table = dst_table
        self.commit_table = dst_table + ".processed"

    def data(self):
        return yt.read_table(self.dst_table, format="json")

    def committed(self):
        return yt.exists(self.commit_table)

    def commit(self):
        yt.create_table(self.commit_table)


def describe_slow_requests(
        log_table,
        output_path,
        yt_proxy,
        expiration_time=None,
        overwrite=False,
        logger=logging.getLogger(),
        tag=None):
    logger = logger.getChild('describe-slow-requests')
    aggregation_key = ["Request", "MediaKind", "Track"]
    job_io = {"table_writer": {"max_key_weight": 256 * 1024}}
    spec = {
        "partition_job_io": job_io,
        "merge_job_io": job_io,
        "sort_job_io": job_io,
    }

    yt.config["proxy"]["url"] = yt_proxy
    yt.config["pool"] = "cloud-nbs"

    log_table_name = os.path.basename(log_table)
    dst_table = os.path.join(output_path, log_table_name)

    if tag:
        dst_table = dst_table + "." + tag

    dst_table = dst_table + ".trace_descrs"
    tmp_table = dst_table + ".all"

    if not yt.exists(log_table) or yt.is_empty(log_table):
        return None

    if yt.exists(dst_table) and not yt.is_empty(dst_table) and not overwrite:
        logger.info("Table {} already exists, aborting".format(dst_table))
        return DescribeSlowRequestsResult(dst_table)

    if not yt.exists(tmp_table) or yt.is_empty(tmp_table) or overwrite:
        logger.info("Map: {} -> {}".format(log_table, tmp_table))
        yt.run_map(
            lambda row: log_line2track(row, tag),
            source_table=log_table,
            destination_table=tmp_table
        )

        logger.info("Sort: {}".format(tmp_table))
        yt.run_sort(
            tmp_table,
            sort_by=aggregation_key,
            spec=spec
        )

        add_expiration_time(tmp_table, expiration_time)

    logger.info("Reduce: {} -> {}".format(tmp_table, dst_table))
    spec_builder = yt.spec_builders.ReduceSpecBuilder() \
        .input_table_paths(tmp_table)                   \
        .output_table_paths(dst_table)                  \
        .reduce_by(aggregation_key)                     \
        .begin_reducer()                                \
        .command(aggregate_tracks)                      \
        .memory_limit(4 * yt.common.GB)                 \
        .end_reducer()
    yt.run_operation(spec_builder)

    logger.info("Sort: {}".format(dst_table))
    yt.run_sort(
        dst_table,
        sort_by=aggregation_key,
        spec=spec
    )

    add_expiration_time(dst_table, expiration_time)

    return DescribeSlowRequestsResult(dst_table)

#
#   Simply dumping all tracks
#


def log_line2trace(row, tag):
    trace = tracelib.extract_trace(row, tag)
    if trace is None:
        return

    # TODO: full probe name (name + request)
    yield {
        "Key": "%s:%s:%s" % (
            trace.request,
            tracelib.media_kind2str(trace.media_kind),
            "->".join([x.name for x in trace.probes])
        ),
        "Value": 1
    }


def uniq(key, rows):
    count = 0
    for row in rows:
        count += 1
    result = {
        "Count": count
    }
    result.update(key)
    yield result


def extract_traces(
        log_table,
        output_path,
        yt_proxy,
        expiration_time=None,
        logger=logging.getLogger(),
        tag=None):
    logger = logger.getChild('extract-traces')
    yt.config["proxy"]["url"] = yt_proxy

    log_table_name = os.path.basename(log_table)
    dst_table = os.path.join(output_path, log_table_name)

    if tag:
        dst_table = dst_table + "." + tag

    dst_table = dst_table + ".traces"
    tmp_table = dst_table + ".tmp"

    logger.info("Map: {} -> {}".format(log_table, tmp_table))
    yt.run_map(
        lambda row: log_line2trace(row, tag),
        source_table=log_table,
        destination_table=tmp_table
    )

    logger.info("Sort: {}".format(tmp_table))
    yt.run_sort(
        tmp_table,
        sort_by=["Key"]
    )

    add_expiration_time(tmp_table, expiration_time)

    logger.info("Reduce: {} -> {}".format(tmp_table, dst_table))
    yt.run_reduce(
        uniq,
        source_table=tmp_table,
        destination_table=dst_table,
        reduce_by=["Key"]
    )

    logger.info("Sort: {}".format(dst_table))
    yt.run_sort(
        dst_table,
        sort_by=["Key"]
    )

    add_expiration_time(dst_table, expiration_time)
