import os
import pytest
import rename_metric

from experiment_pool import MetricColoring
from experiment_pool import MetricValueType

import experiment_pool.pool_helpers as pool_helpers


def get_pool_path(project_path, name):
    return os.path.join(project_path, "tests/ut_data/rename_metric", name)


def try_load_pool(project_path, name):
    return pool_helpers.load_pool(get_pool_path(project_path, name))


# noinspection PyClassHasNoInit
class TestRenameMetric:
    def test_check_not_unique_metric_name(self, project_path):
        pool = try_load_pool(project_path, name="not_unique_metric_name.json")
        with pytest.raises(Exception):
            rename_metric.check_unique_metric(pool)

    def test_check_unique_metric_name(self, project_path):
        pool = try_load_pool(project_path, name="unique_metric_name.json")
        rename_metric.check_unique_metric(pool)

    def test_rename_metric(self, project_path):
        pool = try_load_pool(project_path, name="rename_test.json")
        rename_metric.change_metric_attributes(pool,
                                               new_name="new_metric_name",
                                               new_hname="новое имя метрики",
                                               new_coloring=MetricColoring.LESS_IS_BETTER,
                                               new_value_type=MetricValueType.SUM)

        assert len(pool.observations) == 1
        obs = pool.observations[0]
        assert len(obs.control.metric_results) == 1
        control_mr = obs.control.metric_results[0]
        assert control_mr.metric_key.name == "new_metric_name"
        assert control_mr.ab_info.hname == "новое имя метрики"
        assert control_mr.metric_key.kwargs_name == ""
        assert control_mr.coloring == MetricColoring.LESS_IS_BETTER
        assert control_mr.metric_values.value_type == MetricValueType.SUM

        assert len(obs.experiments) == 1
        assert len(obs.experiments[0].metric_results) == 1
        exp_mr = obs.experiments[0].metric_results[0]
        assert exp_mr.metric_key.name == "new_metric_name"
        assert exp_mr.ab_info.hname == "новое имя метрики"
        assert exp_mr.metric_key.kwargs_name == ""
        assert exp_mr.coloring == MetricColoring.LESS_IS_BETTER
        assert exp_mr.metric_values.value_type == MetricValueType.SUM
