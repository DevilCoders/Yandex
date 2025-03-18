import datetime

import experiment_pool.filter_metric as filter_metric
import yaqutils.time_helpers as utime
from experiment_pool import Experiment
from experiment_pool import MetricDataType
from experiment_pool import MetricResult
from experiment_pool import MetricType
from experiment_pool import MetricValues
from experiment_pool import Observation
from experiment_pool import Pool
from user_plugins import PluginKey

TEST_METRIC_RESULT_1_1 = MetricResult(
    PluginKey("metric1", "kw1"),
    MetricType.ONLINE,
    MetricValues(
        1,
        1.0,
        1.0,
        MetricDataType.VALUES,
    ),
)
TEST_METRIC_RESULT_1_2 = MetricResult(
    PluginKey("metric2", "kw2"),
    MetricType.ONLINE,
    MetricValues(
        2,
        3.0,
        1.5,
        MetricDataType.VALUES,
    ),
)
TEST_METRIC_RESULT_2_1 = MetricResult(
    PluginKey("metric1", "kw3"),
    MetricType.ONLINE,
    MetricValues(
        3,
        6.0,
        2.0,
        MetricDataType.VALUES,
    ),
)
TEST_METRIC_RESULT_2_2 = MetricResult(
    PluginKey("metric2", "kw4"),
    MetricType.ONLINE,
    MetricValues(
        4,
        10.0,
        2.5,
        MetricDataType.VALUES,
    ),
)


# noinspection PyClassHasNoInit
class TestFilterMetric:
    def test_metric_names_white(self):
        exp1 = Experiment(testid="1", metric_results=[TEST_METRIC_RESULT_1_1, TEST_METRIC_RESULT_1_2])
        exp2 = Experiment(testid="2", metric_results=[TEST_METRIC_RESULT_2_1, TEST_METRIC_RESULT_2_2])

        dates = utime.DateRange(datetime.date(2016, 1, 1), datetime.date(2016, 1, 10))
        obs = Observation(obs_id="1", dates=dates, control=exp1, experiments=[exp2])
        pool = Pool(observations=[obs])

        mf = filter_metric.MetricFilter(
            metric_whitelist=["metric1", "metric3"],
        )
        mf.filter_metric_for_pool(pool)

        assert len(pool.observations) == 1
        assert pool.observations[0] is obs
        assert obs.control is exp1
        assert len(obs.experiments) == 1
        assert obs.experiments[0] is exp2

        assert len(exp1.metric_results) == 1
        assert exp1.metric_results[0] is TEST_METRIC_RESULT_1_1
        assert len(exp2.metric_results) == 1
        assert exp2.metric_results[0] is TEST_METRIC_RESULT_2_1

    def test_metric_names_black(self):
        exp1 = Experiment(testid="1", metric_results=[TEST_METRIC_RESULT_1_1, TEST_METRIC_RESULT_1_2])
        exp2 = Experiment(testid="2", metric_results=[TEST_METRIC_RESULT_2_1, TEST_METRIC_RESULT_2_2])

        dates = utime.DateRange(datetime.date(2016, 1, 1), datetime.date(2016, 1, 10))
        obs = Observation(obs_id="1", dates=dates, control=exp1, experiments=[exp2])
        pool = Pool(observations=[obs])

        mf = filter_metric.MetricFilter(
            metric_blacklist=["metric1", "metric3"],
        )
        mf.filter_metric_for_pool(pool)

        assert len(pool.observations) == 1
        assert pool.observations[0] is obs
        assert obs.control is exp1
        assert len(obs.experiments) == 1
        assert obs.experiments[0] is exp2

        assert len(exp1.metric_results) == 1
        assert exp1.metric_results[0] is TEST_METRIC_RESULT_1_2
        assert len(exp2.metric_results) == 1
        assert exp2.metric_results[0] is TEST_METRIC_RESULT_2_2

    def test_metric_names_both(self):
        exp1 = Experiment(testid="1", metric_results=[TEST_METRIC_RESULT_1_1, TEST_METRIC_RESULT_1_2])
        exp2 = Experiment(testid="2", metric_results=[TEST_METRIC_RESULT_2_1, TEST_METRIC_RESULT_2_2])

        dates = utime.DateRange(datetime.date(2016, 1, 1), datetime.date(2016, 1, 10))
        obs = Observation(obs_id="1", dates=dates, control=exp1, experiments=[exp2])
        pool = Pool(observations=[obs])

        mf = filter_metric.MetricFilter(
            metric_whitelist=["metric1", "metric3"],
            metric_blacklist=["metric2", "metric4"],
        )
        mf.filter_metric_for_pool(pool)

        assert len(pool.observations) == 1
        assert pool.observations[0] is obs
        assert obs.control is exp1
        assert len(obs.experiments) == 1
        assert obs.experiments[0] is exp2

        assert len(exp1.metric_results) == 1
        assert exp1.metric_results[0] is TEST_METRIC_RESULT_1_1
        assert len(exp2.metric_results) == 1
        assert exp2.metric_results[0] is TEST_METRIC_RESULT_2_1
