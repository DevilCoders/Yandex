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


def test_filters_metric(yt_stuff, monkeypatch):
    monkeypatch.setattr(squeeze_versions, "get_python_version", lambda: None)
    monkeypatch.setattr(squeeze_versions, "get_revision", lambda: None)

    _run_filters_metric = Runner(yt_stuff.get_yt_client())

    result = _run_filters_metric(batch_file="batch.json",
                                 pool_file="pool.json",
                                 services=["web"])

    _run_filters_metric.check_result_dir()
    _run_filters_metric.check_result_archive()

    result_keys = list(result.keys())
    result_values = list(result.values())

    assert len(result) == 1
    assert result_keys[0].testid == "143353"
    assert result_keys[0].dates == utime.DateRange(utime.parse_date_msk("20190531"), utime.parse_date_msk("20190601"))

    assert len(result_values[0]) == 4
    filter_names = ["User",
                    "Day",
                    "FromFirstDay",
                    "FromFirstTS"]
    filter_count = [3, 3, 3, 3]
    filter_sum = [18, 12, 15, 11]
    filter_average = [6, 4, 5, 3.66]
    filter_rows = [3, 3, 3, 3]

    for i in range(4):
        assert str(result_values[0][i].metric_key) == \
            "<test_metrics.default_metric.ActionCounter{}>".format(filter_names[i])
        assert result_values[0][i].metric_values.count_val == filter_count[i]
        assert (result_values[0][i].metric_values.sum_val - filter_sum[i]) < 1e-2
        assert (result_values[0][i].metric_values.average_val - filter_average[i]) < 1e-2

        assert result_values[0][i].metric_values.data_type == MetricDataType.VALUES
        assert result_values[0][i].metric_values.data_file == "0_exp_143353_20190531_20190601_metric_{}.tsv".format(i)
        assert result_values[0][i].metric_values.row_count == filter_rows[i]
        assert result_values[0][i].metric_values.value_type == MetricValueType.SUM

    return yatest.common.canonical_dir(_run_filters_metric.get_result_dir())
