import importlib
import json
import os
import pytest

import yatest.common
import yt.wrapper

lib = importlib.import_module("cloud.blockstore.tools.analytics.find-perf-bottlenecks.lib")
ytlib = importlib.import_module("cloud.blockstore.tools.analytics.find-perf-bottlenecks.ytlib")

TEST_LOG = "//tmp/log"
OUTPUT_PATH = "//tmp/out"
EXTRA_KEY = "xxx"
INPUT_DATA_PATH = "cloud/blockstore/tools/analytics/find-perf-bottlenecks/tests/data"
INPUT_LOG_PATH = os.path.join(INPUT_DATA_PATH, "log.txt")
INPUT_LOG_LINE_WITH_TRACE_PATH = os.path.join(INPUT_DATA_PATH, "log_line_with_trace.txt")
INPUT_TRACES_PATH = os.path.join(INPUT_DATA_PATH, "traces.txt")


def __yt_proxy():
    return os.environ["YT_PROXY"]


def __setup_yt_test():
    input_data = []
    with open(yatest.common.source_path(INPUT_LOG_PATH)) as f:
        for line in f.readlines():
            input_data.append(json.loads(line.rstrip()))

    yt.wrapper.config["proxy"]["url"] = __yt_proxy()
    yt.wrapper.write_table(TEST_LOG, input_data)

    if not yt.wrapper.exists(OUTPUT_PATH):
        yt.wrapper.create("map_node", OUTPUT_PATH)


def __teardown_yt_test(output_tables):
    result_path = os.path.join(yatest.common.output_path(), "result.txt")
    with open(result_path, "w") as fp:
        for tag, table in output_tables:
            path = os.path.join(OUTPUT_PATH, table)
            rows = []
            for row in yt.wrapper.read_table(path, format="json"):
                rows.append(json.dumps(row, sort_keys=True))
            rows.sort()

            print(tag, file=fp)
            for row in rows:
                print(json.dumps(row, sort_keys=True), file=fp)
    return yatest.common.canonical_file(result_path)


@pytest.mark.parametrize("tag", ["SlowRequests", "AllRequests"])
def test_find_perf_bottlenecks(tag):
    __setup_yt_test()
    ytlib.find_perf_bottlenecks(TEST_LOG, OUTPUT_PATH, __yt_proxy(), tag=tag)
    return __teardown_yt_test([
        ("====PERF====", "log.%s.perf" % tag),
        ("====TOP=====", "log.%s.perf.top" % tag),
    ])


@pytest.mark.parametrize("tag", ["SlowRequests", "AllRequests"])
def test_describe_slow_requests(tag):
    __setup_yt_test()
    result = ytlib.describe_slow_requests(TEST_LOG, OUTPUT_PATH, __yt_proxy(), tag=tag)
    canonical_file = __teardown_yt_test([
        ("===TRACES===", "log.%s.trace_descrs" % tag),
    ])
    assert result is not None
    assert not result.committed()
    assert result.data() is not None
    result.commit()
    assert result.committed()
    return canonical_file


@pytest.mark.parametrize("tag", ["SlowRequests", "AllRequests"])
def test_extract_traces(tag):
    __setup_yt_test()
    ytlib.extract_traces(TEST_LOG, OUTPUT_PATH, __yt_proxy(), tag=tag)
    return __teardown_yt_test([
        ("===TRACES===", "log.%s.traces" % tag),
    ])


def test_build_report():
    with open(yatest.common.source_path(INPUT_TRACES_PATH)) as f:
        rows = [json.loads(x) for x in f.readlines()]

    report = lib.build_report(rows)

    result_path = os.path.join(yatest.common.output_path(), "report.html")
    with open(result_path, "w") as f:
        for request_descr, report_html in report:
            f.write(report_html.decode("utf-8"))
    return yatest.common.canonical_file(result_path)


def test_visualize_trace():
    with open(yatest.common.source_path(INPUT_LOG_LINE_WITH_TRACE_PATH)) as f:
        trace = lib.extract_trace_from_log_message(f.read())
    result_path = os.path.join(yatest.common.output_path(), "trace.html")
    with open(result_path, "w") as f:
        f.write(lib.visualize_trace(trace).decode("utf-8"))
    return yatest.common.canonical_file(result_path)


def test_minimize_traces():
    with open(yatest.common.source_path(INPUT_LOG_PATH)) as f:
        rows = [json.loads(x) for x in f.readlines()]

    result_path = os.path.join(yatest.common.output_path(), "minimized_traces.txt")
    with open(result_path, "w") as f:
        for r in rows:
            for probe in lib.extract_minimized_trace(r).probes:
                f.write(str(probe) + ' ')
            f.write('\n')
    return yatest.common.canonical_file(result_path)
