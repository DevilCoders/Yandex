import pytest

import yaqutils.time_helpers as utime
from experiment_pool import MetricValueType
from experiment_pool import MetricDataType
from mstand_structs import squeeze_versions
from mstand_utils.yt_options_struct import ResultOptions

from mapreduce.yt.python.yt_stuff import YtConfig  # noqa
from test_runner import Runner

import yatest.common


@pytest.fixture(scope="module")
def yt_config(request):
    return YtConfig(local_cypress_dir=yatest.common.work_path("local_cypress_tree"))


def test_default_metric(yt_stuff, monkeypatch):
    monkeypatch.setattr(squeeze_versions, "get_python_version", lambda: None)
    monkeypatch.setattr(squeeze_versions, "get_revision", lambda: None)

    _run_default_metric = Runner(yt_stuff.get_yt_client())

    result = _run_default_metric(batch_file="batch.json",
                                 pool_file="pool.json",
                                 services=["web"])

    _run_default_metric.check_result_dir()
    _run_default_metric.check_result_archive()

    assert len(result) == 2

    keys = sorted(result.keys())

    assert keys[0].testid == "143353"
    assert keys[1].testid == "143355"

    assert keys[0].dates == utime.DateRange(utime.parse_date_msk("20190531"), utime.parse_date_msk("20190531"))
    assert keys[1].dates == utime.DateRange(utime.parse_date_msk("20190531"), utime.parse_date_msk("20190531"))

    assert len(result.values()) == 2

    values = [result[key] for key in keys]

    assert str(values[0][0].metric_key) == "<test_metrics.default_metric.ActionCounter>"
    assert str(values[1][0].metric_key) == "<test_metrics.default_metric.ActionCounter>"

    assert values[0][0].metric_values.count_val == 1
    assert values[1][0].metric_values.count_val == 1

    assert (values[0][0].metric_values.sum_val - 10) < 1e-2
    assert (values[1][0].metric_values.sum_val - 3) < 1e-2

    assert (values[0][0].metric_values.average_val - 10) < 1e-2
    assert (values[1][0].metric_values.average_val - 3) < 1e-2

    assert values[0][0].metric_values.data_type == MetricDataType.VALUES
    assert values[1][0].metric_values.data_type == MetricDataType.VALUES

    assert values[0][0].metric_values.data_file == "0_exp_143353_20190531_20190531_metric_0.tsv"
    assert values[1][0].metric_values.data_file == "1_exp_143355_20190531_20190531_metric_0.tsv"

    assert values[0][0].metric_values.row_count == 1
    assert values[1][0].metric_values.row_count == 1

    assert values[0][0].metric_values.value_type == MetricValueType.SUM
    assert values[1][0].metric_values.value_type == MetricValueType.SUM

    return yatest.common.canonical_dir(_run_default_metric.get_result_dir())


def test_default_metric_split(yt_stuff, monkeypatch):
    monkeypatch.setattr(squeeze_versions, "get_python_version", lambda: None)
    monkeypatch.setattr(squeeze_versions, "get_revision", lambda: None)

    _run_default_metric_split = Runner(yt_stuff.get_yt_client())

    result = _run_default_metric_split(batch_file="batch.json",
                                       pool_file="pool.json",
                                       services=["web"],
                                       split_metric_results=True)

    _run_default_metric_split.check_result_dir()
    _run_default_metric_split.check_result_archive()

    assert len(result) == 2

    keys = sorted(result.keys())

    assert keys[0].testid == "143353"
    assert keys[1].testid == "143355"

    assert keys[0].dates == utime.DateRange(utime.parse_date_msk("20190531"), utime.parse_date_msk("20190531"))
    assert keys[1].dates == utime.DateRange(utime.parse_date_msk("20190531"), utime.parse_date_msk("20190531"))

    assert len(result.values()) == 2

    values = [result[key] for key in keys]

    assert str(values[0][0].metric_key) == "<test_metrics.default_metric.ActionCounter>"
    assert str(values[1][0].metric_key) == "<test_metrics.default_metric.ActionCounter>"

    assert values[0][0].metric_values.count_val == 1
    assert values[1][0].metric_values.count_val == 1

    assert (values[0][0].metric_values.sum_val - 10) < 1e-2
    assert (values[1][0].metric_values.sum_val - 3) < 1e-2

    assert (values[0][0].metric_values.average_val - 10) < 1e-2
    assert (values[1][0].metric_values.average_val - 3) < 1e-2

    assert values[0][0].metric_values.data_type == MetricDataType.VALUES
    assert values[1][0].metric_values.data_type == MetricDataType.VALUES

    assert values[0][0].metric_values.data_file == "0_exp_143353_20190531_20190531_metric_0.tsv"
    assert values[1][0].metric_values.data_file == "1_exp_143355_20190531_20190531_metric_0.tsv"

    assert values[0][0].metric_values.row_count == 1
    assert values[1][0].metric_values.row_count == 1

    assert values[0][0].metric_values.value_type == MetricValueType.SUM
    assert values[1][0].metric_values.value_type == MetricValueType.SUM

    return yatest.common.canonical_dir(_run_default_metric_split.get_result_dir())


def expect_error(func):
    try:
        func()
        return False
    except:
        return True


def test_default_metric_copy(yt_stuff):
    _run_default_metric = Runner(yt_stuff.get_yt_client())

    result = _run_default_metric(batch_file="batch.json",
                                 pool_file="pool.json",
                                 services=["web"],
                                 result_options=ResultOptions("//home", True, 180))

    expect_error(_run_default_metric.check_result_dir)
    expect_error(_run_default_metric.check_result_archive)

    assert len(result) == 2

    keys = sorted(result.keys())

    assert keys[0].testid == "143353"
    assert keys[1].testid == "143355"

    assert keys[0].dates == utime.DateRange(utime.parse_date_msk("20190531"), utime.parse_date_msk("20190531"))
    assert keys[1].dates == utime.DateRange(utime.parse_date_msk("20190531"), utime.parse_date_msk("20190531"))

    assert len(result.values()) == 2

    values = [result[key] for key in keys]

    assert str(values[0][0].metric_key) == "<test_metrics.default_metric.ActionCounter>"
    assert str(values[1][0].metric_key) == "<test_metrics.default_metric.ActionCounter>"

    assert values[0][0].metric_values.count_val == 0
    assert values[1][0].metric_values.count_val == 0

    assert values[0][0].metric_values.sum_val == 0
    assert values[1][0].metric_values.sum_val == 0

    assert values[0][0].metric_values.average_val is None
    assert values[1][0].metric_values.average_val is None

    assert values[0][0].metric_values.data_type == MetricDataType.VALUES
    assert values[1][0].metric_values.data_type == MetricDataType.VALUES

    assert values[0][0].metric_values.data_file is None
    assert values[1][0].metric_values.data_file is None

    assert values[0][0].metric_values.row_count == 0
    assert values[1][0].metric_values.row_count == 0

    assert values[0][0].metric_values.value_type == MetricValueType.SUM
    assert values[1][0].metric_values.value_type == MetricValueType.SUM

    assert values[0][0].result_table_path
    assert values[1][0].result_table_path

    _run_default_metric.check_exists(values[0][0].result_table_path)
    _run_default_metric.check_exists(values[1][0].result_table_path)

    assert values[0][0].result_table_field == "m_0"
    assert values[1][0].result_table_field == "m_0"
