import yt.wrapper as yt

from collections import defaultdict
import importlib
import logging
import os

from cloud.blockstore.tools.analytics.common import add_expiration_time

tracelib = importlib.import_module(
    "cloud.blockstore.tools.analytics.find-perf-bottlenecks.lib")


#
#   Finding worst pairs of adjacent probes
#

def log_line2diffs_between_probes(row, tag):
    trace = tracelib.extract_trace(row, tag)
    if trace is None:
        return

    trace = tracelib.filter_postponed_interval(trace)

    first_probe = trace.probes[0]

    #
    #   T ------------------------------------>
    #       A -> B ->    C1 -> D1        -> E
    #              \                     ^
    #               > C2 ->          D2 /
    #
    #   T --A----B----C2-C1----D1----D2-----E->
    #
    #   (A, B) = 4
    #   (B, C_first) = 4
    #   (B, C_last) = 5
    #   (C_first, D_first) = 5
    #   (C_last, D_first) = 4
    #   (C_first, D_last) = 9
    #   (C_last, D_last) = 8
    #   (D_first, E) = 9
    #   (D_last, E) = 5
    #
    #   (A, B) -> T(A, B)
    #   (B, C_first) -> min(T(B, C1), T(B, C2))
    #   (B, C_last) -> max(T(B, C1), T(B, C2))
    #   (C_first, D_last) -> max(T(Ci, Dj))
    #
    #   adjacent_probes = [(A,B), (B,C), (C,D), (D,E)]
    #   for each adjacent probe pair:
    #       A -> A if len(As) == 1 else [A_first, A_last]
    #       B -> B if len(Bs) == 1 else [B_first, B_last]
    #       calc time diffs for at most 4 combinations: (A_{first,last}, B_{first,last})
    #

    last_probe_name = first_probe.name
    i = 1
    adjacent_probes = []
    probe2times = defaultdict(list)
    probe2times[last_probe_name].append(first_probe.ts)

    while i < len(trace.probes):
        probe = trace.probes[i]
        i += 1
        probe2times[probe.name].append(probe.ts)

        if probe.name == last_probe_name:
            continue

        adjacent_probes.append((last_probe_name, probe.name))
        last_probe_name = probe.name

    for name1, name2 in adjacent_probes:
        times1 = probe2times[name1]
        times2 = probe2times[name2]

        results = []

        if len(times1) == 1:
            if len(times2) == 1:
                results.append(("SingleSingle", times2[0] - times1[0]))
            else:
                results.append(("SingleFirst", times2[0] - times1[0]))
                results.append(("SingleLast", times2[-1] - times1[0]))
        else:
            if len(times2) == 1:
                results.append(("FirstSingle", times2[0] - times1[0]))
                results.append(("LastSingle", times2[0] - times1[-1]))
            else:
                results.append(("FirstFirst", times2[0] - times1[0]))
                results.append(("FirstLast", times2[-1] - times1[0]))
                results.append(("LastFirst", times2[0] - times1[-1]))
                results.append(("LastLast", times2[-1] - times1[-1]))

        for result in results:
            out = {
                "Probe1": name1,
                "Probe2": name2,
                "Request": trace.request,
                "MediaKind": trace.media_kind,
                "Tag": result[0],
                "Time": result[1]
            }
            yield out


def diff_between_probes2percentiles(key, rows):
    times = []
    for row in rows:
        times.append(row["Time"])
    times.sort()
    out = {
        "Time": {}
    }
    for p in [50, 95, 99, 99.9]:
        out["Time"]["P%s" % p] = tracelib.percentile(times, p)
    out.update(key)
    yield out


def find_perf_bottlenecks(
        log_table,
        output_path,
        yt_proxy,
        expiration_time=None,
        overwrite=False,
        logger=logging.getLogger(),
        tag=None):
    logger = logger.getChild('find-perf-bottlenecks')
    aggregation_key = ["Probe1", "Probe2", "Request", "MediaKind", "Tag"]

    yt.config["proxy"]["url"] = yt_proxy
    yt.config["pool"] = "cloud-nbs"

    log_table_name = os.path.basename(log_table)
    dst_table = os.path.join(output_path, log_table_name)

    if tag:
        dst_table = dst_table + "." + tag

    dst_table = dst_table + ".perf"
    diffs_table = dst_table + ".diffs"
    top_table = dst_table + ".top"

    if not yt.exists(log_table) or yt.is_empty(log_table):
        return

    if yt.exists(top_table) and not yt.is_empty(top_table) and not overwrite:
        logger.info("Table {} already exists, aborting".format(top_table))
        return

    logger.info("Map: {} -> {}".format(log_table, diffs_table))
    yt.run_map(
        lambda row: log_line2diffs_between_probes(row, tag),
        source_table=log_table,
        destination_table=diffs_table
    )

    logger.info("Sort: {}".format(diffs_table))
    yt.run_sort(
        diffs_table,
        sort_by=aggregation_key
    )

    add_expiration_time(diffs_table, expiration_time)

    logger.info("Reduce: {} -> {}".format(diffs_table, dst_table))
    spec_builder = yt.spec_builders.ReduceSpecBuilder() \
        .input_table_paths(diffs_table)                 \
        .output_table_paths(dst_table)                  \
        .reduce_by(aggregation_key)                     \
        .begin_reducer()                                \
        .command(diff_between_probes2percentiles)       \
        .memory_limit(4 * yt.common.GB)                 \
        .end_reducer()
    yt.run_operation(spec_builder)

    logger.info("Sort: {}".format(dst_table))
    yt.run_sort(
        dst_table,
        sort_by=aggregation_key
    )

    add_expiration_time(dst_table, expiration_time)

    logger.info("Read: {}".format(dst_table))
    key2result = defaultdict(list)
    for row in yt.read_table(dst_table, format="json"):
        spec = {}
        for k, v in row.items():
            if k not in ["Time", "Request", "MediaKind"]:
                spec[k] = v

        time = row["Time"]
        request = row["Request"]

        media_kind = tracelib.media_kind2str(row["MediaKind"])
        for p, t in time.items():
            key2result["%s:%s:%s" % (request, media_kind, p)].append((t, spec))

    tops = []
    for key, result in key2result.items():
        result.sort(key=lambda x: -x[0])
        tops.append({
            "Key": key,
            "Result": result[0:10],
        })

    yt.write_table(top_table, tops)
    logger.info("Sort: {}".format(top_table))
    yt.run_sort(
        top_table,
        sort_by=["Key"]
    )

    add_expiration_time(top_table, expiration_time)
