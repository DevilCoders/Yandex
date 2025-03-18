import pytest

import yaqutils.time_helpers as utime
from mstand_structs import squeeze_versions
from session_metric.metric_calculator import NUM_BUCKETS
from experiment_pool import MetricValueType
from experiment_pool import MetricDataType

from mapreduce.yt.python.yt_stuff import YtConfig  # noqa
from test_runner import Runner

import yatest.common


@pytest.fixture(scope="module")
def yt_config(request):
    return YtConfig(local_cypress_dir=yatest.common.work_path("local_cypress_tree"))


def test_bucket_metric(yt_stuff, monkeypatch):
    monkeypatch.setattr(squeeze_versions, "get_python_version", lambda: None)
    monkeypatch.setattr(squeeze_versions, "get_revision", lambda: None)

    _run_bucket_metric = Runner(yt_stuff.get_yt_client())

    result = _run_bucket_metric(batch_file="batch.json",
                                pool_file="pool.json",
                                services=["web"],
                                use_buckets=True)

    _run_bucket_metric.check_result_dir()
    _run_bucket_metric.check_result_archive()

    result_keys = list(result.keys())
    result_values = list(result.values())

    assert len(result) == 1
    assert result_keys[0].testid == "98779"
    assert result_keys[0].dates == utime.DateRange(utime.parse_date_msk("20181005"), utime.parse_date_msk("20181005"))
    assert len(result_values) == 1
    assert str(result_values[0][0].metric_key) == "<test_metrics.default_metric.ActionCounter>"
    assert result_values[0][0].metric_values.count_val == NUM_BUCKETS
    assert (result_values[0][0].metric_values.sum_val - 128) < 1e-2
    assert (result_values[0][0].metric_values.average_val - 128. / NUM_BUCKETS) < 1e-2
    assert result_values[0][0].metric_values.data_type == MetricDataType.VALUES
    assert result_values[0][0].metric_values.data_file == "0_exp_98779_20181005_20181005_metric_0.tsv"
    assert result_values[0][0].metric_values.row_count == 100
    assert result_values[0][0].metric_values.value_type == MetricValueType.SUM

    return yatest.common.canonical_dir(_run_bucket_metric.get_result_dir())


def test_bucket_metric_split(yt_stuff, monkeypatch):
    monkeypatch.setattr(squeeze_versions, "get_python_version", lambda: None)
    monkeypatch.setattr(squeeze_versions, "get_revision", lambda: None)

    _run_bucket_metric_split = Runner(yt_stuff.get_yt_client())

    result = _run_bucket_metric_split(batch_file="batch.json",
                                      pool_file="pool.json",
                                      services=["web"],
                                      use_buckets=True,
                                      split_metric_results=True)

    _run_bucket_metric_split.check_result_dir()
    _run_bucket_metric_split.check_result_archive()

    result_keys = list(result.keys())
    result_values = list(result.values())

    assert len(result) == 1
    assert result_keys[0].testid == "98779"
    assert result_keys[0].dates == utime.DateRange(utime.parse_date_msk("20181005"), utime.parse_date_msk("20181005"))
    assert len(result_values) == 1
    assert str(result_values[0][0].metric_key) == "<test_metrics.default_metric.ActionCounter>"
    assert result_values[0][0].metric_values.count_val == NUM_BUCKETS
    assert (result_values[0][0].metric_values.sum_val - 128) < 1e-2
    assert (result_values[0][0].metric_values.average_val - 128. / NUM_BUCKETS) < 1e-2
    assert result_values[0][0].metric_values.data_type == MetricDataType.VALUES
    assert result_values[0][0].metric_values.data_file == "0_exp_98779_20181005_20181005_metric_0.tsv"
    assert result_values[0][0].metric_values.row_count == 100
    assert result_values[0][0].metric_values.value_type == MetricValueType.SUM

    return yatest.common.canonical_dir(_run_bucket_metric_split.get_result_dir())
