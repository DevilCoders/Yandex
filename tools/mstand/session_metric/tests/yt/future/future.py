import pytest

import yaqutils.time_helpers as utime
from experiment_pool import MetricValueType
from experiment_pool import MetricDataType
from mstand_structs import squeeze_versions

from mapreduce.yt.python.yt_stuff import YtConfig  # noqa
from test_runner import Runner

import yatest.common


@pytest.fixture(scope="module")
def yt_config(request):
    return YtConfig(local_cypress_dir=yatest.common.work_path("local_cypress_tree"))


def test_future_metric(yt_stuff, monkeypatch):
    monkeypatch.setattr(squeeze_versions, "get_python_version", lambda: None)
    monkeypatch.setattr(squeeze_versions, "get_revision", lambda: None)

    _run_future_metric = Runner(yt_stuff.get_yt_client())

    result = _run_future_metric(batch_file="batch.json",
                                pool_file="pool.json",
                                services=["web"],
                                future=2)

    _run_future_metric.check_result_dir()
    _run_future_metric.check_result_archive()

    result_keys = list(result.keys())
    result_values = list(result.values())

    assert len(result) == 1
    assert result_keys[0].testid == "98779"
    assert result_keys[0].dates == utime.DateRange(utime.parse_date_msk("20181005"), utime.parse_date_msk("20181005"))
    assert len(result_values) == 1
    assert str(result_values[0][0].metric_key) == "<test_metrics.default_metric.ActionCounterFuture>"
    assert result_values[0][0].metric_values.count_val == 3
    assert (result_values[0][0].metric_values.sum_val - 93) < 1e-2
    assert (result_values[0][0].metric_values.average_val - 31.) < 1e-2
    assert result_values[0][0].metric_values.data_type == MetricDataType.VALUES
    assert result_values[0][0].metric_values.data_file == "0_exp_98779_20181005_20181005_metric_0.tsv"
    assert result_values[0][0].metric_values.row_count == 3
    assert result_values[0][0].metric_values.value_type == MetricValueType.SUM

    return yatest.common.canonical_dir(_run_future_metric.get_result_dir())
