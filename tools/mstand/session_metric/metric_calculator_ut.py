import datetime

import session_metric.metric_calculator as m_c
import yaqutils.time_helpers as utime
from experiment_pool import Experiment
from experiment_pool import MetricColoring
from experiment_pool import MetricValueType
from experiment_pool import Observation
from user_plugins import PluginContainer
from user_plugins import PluginKey


# noinspection PyClassHasNoInit
class TestMetricSources:
    def test_present(self):
        tables = ["1", "2", "3"]
        sources = m_c.MetricSources(tables, [], [])
        assert sources.all_tables() == tables
        infos = list(sources.all_tables_info())
        assert [info.table for info in infos] == tables
        for info in infos:
            assert not info.history
            assert not info.future

    def test_history_and_future(self):
        tables = ["source"] * 14
        tables_history = ["history"] * 10
        tables_future = ["future"] * 5
        sources = m_c.MetricSources(tables, tables_history, tables_future)
        assert sources.all_tables() == tables + tables_history + tables_future
        infos = list(sources.all_tables_info())
        assert [info.table for info in infos] == tables + tables_history + tables_future
        for info in infos:
            if info.history:
                assert info.table == "history"
            elif info.future:
                assert info.table == "future"
            else:
                assert info.table == "source"

    def test_multiple_buckets_for_one_user(self):
        class BucketNumberMetric(object):
            value_type = MetricValueType.SUM
            coloring = MetricColoring.MORE_IS_BETTER

            def __call__(self, actions):
                return actions.actions[0].data["bucket"]

        container = PluginContainer.create_single_direct(PluginKey("test_metric"), BucketNumberMetric())

        dates = utime.DateRange(datetime.date(2016, 5, 2), datetime.date(2016, 5, 2))
        exp = Experiment(testid="test")
        obs = Observation("test", dates, control=exp)

        rows = [
            {"yuid": "y1234", "testid": "1", "ts": 0, "bucket": 1},
            {"yuid": "y1234", "testid": "1", "ts": 1, "bucket": 2},
            {"yuid": "y1234", "testid": "1", "ts": 2, "bucket": 3},
            {"yuid": "y1234", "testid": "1", "ts": 3, "bucket": 1},
        ]
        rows_iter = iter(rows)
        """:type rows_iter: __generator[dict[str]]"""
        result = m_c.calc_metric_for_user_impl(
            metric_container=container,
            yuid="y1234",
            rows=rows_iter,
            experiment=exp,
            observation=obs,
            use_buckets=True
        )

        assert result is None
