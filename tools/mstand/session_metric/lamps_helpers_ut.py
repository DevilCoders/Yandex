import datetime

import session_metric.lamps_helpers as ulamps
import yaqutils.time_helpers as utime
from experiment_pool import Pool
from experiment_pool import Observation
from experiment_pool import Experiment
from experiment_pool import MetricResult
from experiment_pool import MetricValues
from experiment_pool import MetricDataType
from experiment_pool import MetricType

from mstand_structs import LampKey
from mstand_structs import SqueezeVersions
from user_plugins import PluginKey


class TestLampPoolConversion(object):
    version = SqueezeVersions({"web": 1}, 1, 2)
    metric_key = PluginKey(name="metric")
    metric_values = MetricValues(average_val=0.5, count_val=10, sum_val=5, data_type=MetricDataType.VALUES)
    metric_result = MetricResult(metric_key=metric_key, metric_values=metric_values,
                                 metric_type=MetricType.ONLINE, version=version)

    dates = utime.DateRange(datetime.date(2016, 10, 17), datetime.date(2016, 12, 23))
    control1 = Experiment(testid="100", metric_results=[metric_result])
    exp11 = Experiment(testid="101", metric_results=[metric_result])
    exp12 = Experiment(testid="102", metric_results=[metric_result])
    experiments1 = [exp11, exp12]
    obs_dates1 = dates
    obs1 = Observation(obs_id="1001",
                       dates=obs_dates1,
                       control=control1,
                       experiments=experiments1)

    obs2 = Observation(obs_id="1002",
                       dates=obs_dates1,
                       control=control1,
                       experiments=experiments1)

    obs3 = Observation(obs_id="1001",
                       dates=obs_dates1,
                       control=control1,
                       experiments=[exp11])

    pool1 = Pool(observations=[obs1])
    pool2 = Pool(observations=[obs2])
    pool3 = Pool(observations=[obs3])

    lamp1 = LampKey(control="100", testid="100", dates=dates, observation="1001", version=version)
    lamp2 = LampKey(control="100", testid="101", dates=dates, observation="1001", version=version)
    lamp3 = LampKey(control="100", testid="102", dates=dates, observation="1001", version=version)
    lamp4 = LampKey(control="100", testid="102", dates=dates, observation="1002", version=version)

    def test_key_extraction(self):
        keys = ulamps.get_lamps_keys_from_pool(self.pool1)
        assert len(keys) == 3
        assert self.lamp1 in keys
        assert self.lamp2 in keys
        assert self.lamp3 in keys

    def test_pool_from_cache(self):
        cache = [(self.lamp1, [self.metric_result]),
                 (self.lamp2, [self.metric_result]),
                 (self.lamp3, [self.metric_result])]

        new_pool, isempty = ulamps.pool_from_lamps(self.pool2, cache, False)

        assert not isempty
        assert len(new_pool.observations) == 1
        assert new_pool.observations[0].id == "1002"

    def test_partially_cached_lamps(self):
        cache_no_exp = [(self.lamp1, [self.metric_result]),
                        (self.lamp3, [self.metric_result]),
                        (self.lamp4, [self.metric_result])]

        cache_no_control_and_exp = [(self.lamp3, [self.metric_result]),
                                    (self.lamp4, [self.metric_result])]

        cache_no_control = [(self.lamp2, [self.metric_result]),
                            (self.lamp3, [self.metric_result]),
                            (self.lamp4, [self.metric_result])]

        cache_pool_no_exp, isempty = ulamps.pool_from_lamps(self.pool3, cache_no_exp, True)
        assert not isempty
        assert cache_pool_no_exp.observations[0].experiments[0].testid == "100"
        calc_pool_no_exp, isempty = ulamps.pool_from_lamps(self.pool3, cache_no_exp, False)
        assert not isempty
        assert calc_pool_no_exp.observations[0].experiments[0].testid == "101"

        cache_pool_no_control_and_exp, isempty = ulamps.pool_from_lamps(self.pool3, cache_no_control_and_exp, True)
        assert isempty
        calc_pool_no_control_and_exp, isempty = ulamps.pool_from_lamps(self.pool3, cache_no_control_and_exp, False)
        assert not isempty
        assert calc_pool_no_control_and_exp.observations[0].experiments[0].testid == "101"

        cache_pool_no_control, isempty = ulamps.pool_from_lamps(self.pool3, cache_no_control, True)
        assert not isempty
        assert cache_pool_no_control.observations[0].experiments[0].testid == "101"
        calc_pool_no_control, isempty = ulamps.pool_from_lamps(self.pool3, cache_no_control, False)
        assert not isempty
        assert not calc_pool_no_control.observations[0].experiments
