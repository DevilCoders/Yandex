import datetime

import yaqutils.time_helpers as utime
from experiment_pool import Experiment
from experiment_pool import MetricDataType
from experiment_pool import MetricResult
from experiment_pool import LampResult
from experiment_pool import MetricType
from experiment_pool import MetricValues
from experiment_pool import Observation
from experiment_pool import Pool
from user_plugins import PluginKey
from mstand_structs import LampKey
from mstand_structs import SqueezeVersions


# noinspection PyClassHasNoInit
class TestPool:
    def test_pool_serialize(self):
        control1 = Experiment(testid="100")
        exp11 = Experiment(testid="101")
        exp12 = Experiment(testid="102")
        experiments1 = [exp11, exp12]
        obs_dates1 = utime.DateRange(datetime.date(2016, 10, 17), datetime.date(2016, 12, 23))
        obs1 = Observation(obs_id="1001",
                           dates=obs_dates1,
                           control=control1,
                           experiments=experiments1)
        assert (obs1 == obs1)

        control2 = Experiment(testid="200")
        exp21 = Experiment(testid="201")
        exp22 = Experiment(testid="202")

        metric_key = PluginKey(name="metric")
        metric_values = MetricValues(average_val=0.5, count_val=10, sum_val=5, data_type=MetricDataType.VALUES)
        metric_result = MetricResult(metric_key=metric_key, metric_values=metric_values, metric_type=MetricType.ONLINE)

        exp22.add_metric_result(metric_result)
        experiments2 = [exp21, exp22]

        obs_dates2 = utime.DateRange(datetime.date(2013, 10, 17), datetime.date(2013, 12, 23))
        obs2 = Observation(obs_id="2001",
                           dates=obs_dates2,
                           control=control2,
                           experiments=experiments2)

        assert (obs1 != obs2)

        pool = Pool([obs1, obs2])
        ser_pool = pool.serialize()
        assert "version" in ser_pool
        assert "observations" in ser_pool
        ser_observations = ser_pool["observations"]
        assert len(ser_observations) == 2

        assert pool.all_metric_keys() == {metric_key}
        assert pool.all_criteria_keys() == set()
        pool.log_stats()
        pool.log_metric_stats()

        pool = Pool([], extra_data={"extra": "data"})
        assert pool.serialize()["extra_data"] == {"extra": "data"}

        version = SqueezeVersions({"web": 1}, 1, 2)
        lamp = [LampResult(lamp_key=LampKey(control="1337",
                                            observation="100500",
                                            testid="9000",
                                            dates=obs_dates2,
                                            version=version),
                           lamp_values=[metric_result])]
        pool = Pool([], lamps=lamp)
        assert pool.serialize()["lamps"][0]["lamp_key"] == lamp[0].lamp_key.serialize()
        assert pool.serialize()["lamps"][0]["values"] == [metric_result.serialize()]

    def test_fill_sbs_workflow_ids(self):
        obs1 = Observation(
            obs_id="12345",
            control=None,
            experiments=[],
            dates=None,
            sbs_ticket="SIDEBYSIDE-111",
        )
        obs2 = Observation(
            obs_id="67890",
            control=None,
            experiments=[],
            dates=None,
            sbs_ticket="SIDEBYSIDE-222",
        )
        pool = Pool(observations=[obs1, obs2])

        sbs_ticket_workflow_map = {
            "SIDEBYSIDE-111": "aaaa-1111",
            "SIDEBYSIDE-222": "bbbb-2222",
        }
        pool.fill_sbs_workflow_ids(sbs_ticket_workflow_map=sbs_ticket_workflow_map)

        assert obs1.sbs_workflow_id == sbs_ticket_workflow_map[obs1.sbs_ticket]
        assert obs2.sbs_workflow_id == sbs_ticket_workflow_map[obs2.sbs_ticket]
