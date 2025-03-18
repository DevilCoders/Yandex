import datetime
import pytest
import os

import yaqutils.time_helpers as utime
from experiment_pool import Experiment
from experiment_pool import MetricDataType
from experiment_pool import MetricResult
from experiment_pool import MetricType
from experiment_pool import MetricValues
from experiment_pool import Observation
from experiment_pool import Pool
from experiment_pool import LampResult
from experiment_pool import pool_helpers
from experiment_pool.metric_result import SbsMetricResult
from experiment_pool.metric_result import SbsMetricValues
from mstand_structs import SqueezeVersions
from mstand_structs import LampKey
from user_plugins import PluginKey


def create_obs(obs_id, metric_name):
    ctrl = Experiment(testid=obs_id + "0", metric_results=[create_metric_result(metric_name, 10)])
    exp1 = Experiment(testid=obs_id + "1", metric_results=[create_metric_result(metric_name, 11)])
    exp2 = Experiment(testid=obs_id + "2", metric_results=[create_metric_result(metric_name, 12)],
                      extra_data={"exp2": "extra_data"}, errors=["exp2_error"])
    exps = [exp1, exp2]
    dates = utime.DateRange(datetime.date(2013, 10, 17), datetime.date(2013, 12, 23))
    return Observation(obs_id=obs_id, dates=dates, control=ctrl, experiments=exps)


def create_metric_result(metric_name, val):
    metric_key = PluginKey(name=metric_name)
    metric_values = MetricValues(average_val=val, count_val=10, sum_val=10 * val, data_type=MetricDataType.VALUES)
    metric_result = MetricResult(metric_key=metric_key, metric_values=metric_values, metric_type=MetricType.ONLINE)
    return metric_result


@pytest.fixture
def extended_pool():
    main_obs = create_obs("1", "m_name")
    daily_obs = pool_helpers.get_daily_observations(main_obs)
    pool = Pool([main_obs] + daily_obs)
    return pool, main_obs


def path_to_pool(path, name):
    return os.path.join(path, name)


# noinspection PyClassHasNoInit
class TestPoolMerge:
    def test_pool_merge(self):
        pool0 = Pool()

        obs11 = create_obs("1", "m1")
        obs21 = create_obs("2", "m1")
        obs31 = create_obs("3", "m1")
        # now extra_data is the part of a key
        obs31.extra_data = {"test": "extra_data"}
        version = SqueezeVersions({"web": 1}, 1, 2)
        lamp1 = LampResult(LampKey("11", "10", "1", obs11.dates, version), [create_metric_result("lamp1", 13)])
        pool1 = Pool([obs11, obs21, obs31], lamps=[lamp1])

        obs22 = create_obs("2", "m2")
        obs32 = create_obs("3", "m2")
        obs32.extra_data = {"test": "extra_data"}
        obs42 = create_obs("4", "m2")
        pool2_extra_data = {"extra": "data"}
        lamp2 = LampResult(LampKey("21", "20", "2", obs22.dates, version), [create_metric_result("lamp2", 14)])
        pool2 = Pool([obs22, obs32, obs42], extra_data=pool2_extra_data, lamps=[lamp1, lamp2])

        merged = pool_helpers.merge_pools([pool0, pool1, pool2])
        assert merged.synthetic_summaries is None
        assert merged.extra_data == pool2_extra_data
        assert len(merged.lamps) == 2
        assert str(lamp1) in str(merged.lamps)
        assert str(lamp2) in str(merged.lamps)

        assert len(merged.observations) == 4
        merged_obs1, merged_obs2, merged_obs3, merged_obs4 = sorted(merged.observations, key=lambda o: o.id)

        assert merged_obs1.key() == obs11.key()
        assert len(merged_obs1.control.metric_results) == 1
        assert merged_obs1.extra_data is None

        assert merged_obs2.key() == obs21.key()
        assert merged_obs2.key() == obs22.key()
        assert len(merged_obs2.control.metric_results) == 2
        assert merged_obs2.extra_data is None

        assert merged_obs3.key() == obs31.key()
        assert merged_obs3.key() == obs32.key()
        assert len(merged_obs3.control.metric_results) == 2
        assert merged_obs3.extra_data == {"test": "extra_data"}

        assert merged_obs4.key() == obs42.key()
        assert len(merged_obs4.control.metric_results) == 1
        assert merged_obs4.extra_data is None

    @staticmethod
    def _get_testids(obs):
        return tuple(exp.testid for exp in obs.experiments)

    def test_duplicates(self, data_path):
        # MSTAND-1275
        pool1 = pool_helpers.load_pool(path_to_pool(data_path, "pool_for_merge_1.json"))
        pool2 = pool_helpers.load_pool(path_to_pool(data_path, "pool_for_merge_2.json"))
        assert len(pool1.observations) == 1
        assert len(pool2.observations) == 1
        assert pool1.observations[0].id == "34380"
        assert pool2.observations[0].id == "34380"

        merged = pool_helpers.merge_pools([pool1, pool2])
        assert len(merged.observations) == 1
        assert merged.observations[0].id == "34380"
        assert self._get_testids(merged.observations[0]) == self._get_testids(pool1.observations[0])

        merged = pool_helpers.merge_pools([pool2, pool1])
        assert len(merged.observations) == 1
        assert merged.observations[0].id == "34380"
        assert self._get_testids(merged.observations[0]) == self._get_testids(pool2.observations[0])

    @staticmethod
    def _get_metric_keys(exp):
        return tuple(m.key() for m in exp.metric_results)

    def test_metrics_order(self, data_path):
        pool1 = pool_helpers.load_pool(path_to_pool(data_path, "pool_for_merge_with_metrics_1.json"))
        pool2 = pool_helpers.load_pool(path_to_pool(data_path, "pool_for_merge_with_metrics_2.json"))

        assert len(pool1.observations) == 1
        assert len(pool2.observations) == 1
        assert pool1.observations[0].id == pool2.observations[0].id
        assert len(pool1.observations[0].control.metric_results) == 3
        assert len(pool2.observations[0].control.metric_results) == 2

        control_key = pool1.observations[0].control.key()
        metric_keys = self._get_metric_keys(pool1.observations[0].control)
        assert metric_keys == tuple(sorted(metric_keys))

        merged = pool_helpers.merge_pools([pool1, pool2])
        assert len(merged.observations) == 1
        assert merged.observations[0].control.key() == control_key
        assert self._get_metric_keys(merged.observations[0].control) == metric_keys

        merged = pool_helpers.merge_pools([pool2, pool1])
        assert len(merged.observations) == 1
        assert merged.observations[0].control.key() == control_key
        assert self._get_metric_keys(merged.observations[0].control) == metric_keys


# noinspection PyClassHasNoInit
class TestPoolIntersect:
    def test_pool_intersect(self):
        obs11 = create_obs("1", "m1")
        obs21 = create_obs("2", "m1")
        obs31 = create_obs("3", "m1")
        # now extra_data is the part of a key
        obs31.extra_data = {"test": "extra_data"}
        pool1 = Pool([obs11, obs21, obs31])

        obs22 = create_obs("2", "m2")
        obs32 = create_obs("3", "m2")
        obs32.extra_data = {"test": "extra_data"}
        obs42 = create_obs("4", "m2")
        pool2_extra_data = {"extra": "data"}
        pool2 = Pool([obs22, obs32, obs42], extra_data=pool2_extra_data)

        intersection = pool_helpers.intersect_pools([pool1, pool2])
        assert intersection.synthetic_summaries is None
        assert intersection.extra_data == pool2_extra_data

        assert len(intersection.observations) == 2
        merged_obs2, merged_obs3 = sorted(intersection.observations, key=lambda o: o.id)

        assert merged_obs2.key() == obs21.key()
        assert merged_obs2.key() == obs22.key()
        assert len(merged_obs2.control.metric_results) == 2
        assert merged_obs2.extra_data is None

        assert merged_obs3.key() == obs31.key()
        assert merged_obs3.key() == obs32.key()
        assert len(merged_obs3.control.metric_results) == 2
        assert merged_obs3.extra_data == {"test": "extra_data"}


# noinspection PyClassHasNoInit
class TestPoolHelpers:
    def test_enumerate_all_metrics_in_observations_sort_by_name_true(self, data_path):
        pool = pool_helpers.load_pool(path_to_pool(data_path, "enumerate_all_metrics_in_observations.json"))
        metrics = pool_helpers.enumerate_all_metrics_in_observations(pool.observations, sort_by_name=True)
        metrics_names = [metric_name.pretty_name() for metric_name in metrics]
        sorted_metrics_names = sorted(metrics_names)

        assert len(metrics_names) == len(sorted_metrics_names)
        assert metrics_names == sorted_metrics_names

    def test_enumerate_all_metrics_in_observations_sort_by_name_false(self, data_path):
        pool = pool_helpers.load_pool(path_to_pool(data_path, "enumerate_all_metrics_in_observations.json"))
        metrics = pool_helpers.enumerate_all_metrics_in_observations(pool.observations, sort_by_name=False)
        metrics_names = [metric_name.pretty_name() for metric_name in metrics]
        sorted_metrics_names = sorted(metrics_names)

        assert len(metrics_names) == len(sorted_metrics_names)
        assert metrics_names != sorted_metrics_names

    def test_enumerate_lamps_in_observations_sort_by_name_true(self, data_path):
        pool = pool_helpers.load_pool(path_to_pool(data_path, "enumerate_all_metrics_in_observations.json"))
        metrics = pool_helpers.enumerate_all_metrics_in_lamps(pool, sort_by_name=True)
        metrics_names = [metric_name.pretty_name() for metric_name in metrics]
        sorted_metrics_names = sorted(metrics_names)

        assert len(metrics_names) == len(sorted_metrics_names)
        assert metrics_names == sorted_metrics_names

    def test_enumerate_lamps_in_observations_sort_by_name_false(self, data_path):
        pool = pool_helpers.load_pool(path_to_pool(data_path, "enumerate_all_metrics_in_observations.json"))
        metrics = pool_helpers.enumerate_all_metrics_in_lamps(pool, sort_by_name=False)
        metrics_names = [metric_name.pretty_name() for metric_name in metrics]
        sorted_metrics_names = sorted(metrics_names)

        assert len(metrics_names) == len(sorted_metrics_names)
        assert metrics_names != sorted_metrics_names

    def test_empty_lamps_enumerate(self, data_path):
        pool = pool_helpers.load_pool(path_to_pool(data_path, "enumerate_empty_lamps_in_observations.json"))
        metrics = pool_helpers.enumerate_all_metrics_in_lamps(pool, sort_by_name=False)
        metrics_names = [metric_name.pretty_name() for metric_name in metrics]

        assert len(metrics_names) == 0


# noinspection PyClassHasNoInit
class TestSbsMetricsMerge:
    def test_sbs_metrics_merge(self):
        metric_result_a1 = SbsMetricResult(
            metric_key=PluginKey("a"),
            sbs_metric_values=SbsMetricValues(
                single_results=[{"a": 1}],
                pair_results=[{"aa": 11}],
            )
        )
        metric_result_a2 = SbsMetricResult(
            metric_key=PluginKey("a"),
            sbs_metric_values=SbsMetricValues(
                single_results=[{"a": 1}],
                pair_results=[{"aa": 11}],
            )
        )
        metric_result_b = SbsMetricResult(
            metric_key=PluginKey("b"),
            sbs_metric_values=SbsMetricValues(
                single_results=[{"b": 2}],
                pair_results=[{"bb": 22}],
            )
        )
        metric_result_c = SbsMetricResult(
            metric_key=PluginKey("c"),
            sbs_metric_values=SbsMetricValues(
                single_results=[{"c": 3}],
                pair_results=[{"cc": 33}],
            )
        )

        obs1 = Observation(obs_id=None,
                           dates=None,
                           control=Experiment(testid="1"),
                           experiments=[],
                           extra_data="",
                           sbs_ticket="SIDEBYSIDE-0",
                           sbs_workflow_id="z",
                           sbs_metric_results=[metric_result_a1, metric_result_b])

        obs2 = Observation(obs_id=None,
                           dates=None,
                           control=Experiment(testid="1"),
                           experiments=[],
                           extra_data="",
                           sbs_ticket="SIDEBYSIDE-0",
                           sbs_metric_results=[metric_result_a2, metric_result_c])

        pool_helpers.merge_observations(obs1, obs2)
        assert obs2.sbs_workflow_id == obs1.sbs_workflow_id
        results = [result for result in obs2.sbs_metric_results]
        results.sort(key=lambda r: r.metric_key.name)
        values = [result.sbs_metric_values for result in results]
        assert [v.single_results for v in values] == [[{"a": 1}], [{"b": 2}], [{"c": 3}]]
        assert [v.pair_results for v in values] == [[{"aa": 11}], [{"bb": 22}], [{"cc": 33}]]


# noinspection PyClassHasNoInit
class TestSeparateObservations:
    def test_extend_pool_by_daily(self, extended_pool):
        pool, main_obs = extended_pool
        assert len(pool.observations) == main_obs.dates.number_of_days() + 1

    def test_separate_observations(self, extended_pool):
        pool, main_obs_origin = extended_pool
        main_obs, daily_obs = pool_helpers.separate_observations(pool)

        assert main_obs == main_obs_origin

        assert len(daily_obs) == main_obs.dates.number_of_days()
        assert all(obs.dates.number_of_days() == 1 for obs in daily_obs)

        dates = set(obs.dates.start for obs in daily_obs)
        assert len(dates) == main_obs.dates.number_of_days()
        assert min(dates) == main_obs.dates.start
        assert max(dates) == main_obs.dates.end

    def test_separete_observations_without_daily(self, extended_pool):
        main_obs_origin = create_obs("1", "m_name")
        pool = Pool([main_obs_origin])

        main_obs, daily_obs = pool_helpers.separate_observations(pool)

        assert main_obs == main_obs_origin
        assert daily_obs == []

    def test_separete_observations_with_two_daily(self, extended_pool):
        pool, main_obs = extended_pool
        main_obs, daily_obs = pool_helpers.separate_observations(pool)

        bad_pool = Pool([main_obs, main_obs] + daily_obs)

        with pytest.raises(Exception) as exc:
            pool_helpers.separate_observations(bad_pool)
            assert str(exc) == "Pool should contain one main observation only"

    def test_separate_observations_without_main(self, extended_pool):
        pool, main_obs = extended_pool
        main_obs, daily_obs = pool_helpers.separate_observations(pool)

        bad_pool = Pool(daily_obs)
        with pytest.raises(Exception) as exc:
            pool_helpers.separate_observations(bad_pool)
            assert str(exc) == "Pool should contain a main observation"

    def test_separate_observations_have_gaps(self, extended_pool):
        pool, main_obs = extended_pool
        main_obs, daily_obs = pool_helpers.separate_observations(pool)

        bad_pool = Pool([main_obs, daily_obs[0], daily_obs[-1]])
        with pytest.raises(Exception) as exc:
            pool_helpers.separate_observations(bad_pool)
            assert str(exc) == "Daily observations do not cover whole a main observation"

    def test_separate_observations_have_duplicate(self, extended_pool):
        pool, main_obs = extended_pool
        main_obs, daily_obs = pool_helpers.separate_observations(pool)

        daily_obs[1] = daily_obs[0]
        bad_pool = Pool([main_obs] + daily_obs)
        with pytest.raises(Exception) as exc:
            pool_helpers.separate_observations(bad_pool)
            assert str(exc) == "Daily observations do not cover whole a main observation"


# noinspection PyClassHasNoInit
class TestPoolConversionToABFormat:
    def test_necessary_fields_zero_control(self, data_path):
        pool = pool_helpers.load_pool(path_to_pool(data_path, "pool_to_convert_zero_control.json"))
        converted_pool = pool_helpers.convert_pool_to_ab_format(pool)
        assert converted_pool["data"]["metrics"]["abandonment rate"][0]["val"] is not None
        assert converted_pool["data"]["metrics"]["abandonment rate"][1]["val"] is not None
        assert converted_pool["data"]["metrics"]["abandonment rate"][1]["delta_prec"] is not None
        assert converted_pool["data"]["metrics"]["abandonment rate"][1]["pvalue"] is not None
        assert converted_pool["data"]["metrics"]["abandonment rate"][1]["delta_val"] is not None
        assert converted_pool["data"]["metrics"]["abandonment rate"][1].get("delta_percent") is None

    def test_necessary_fields(self, data_path):
        pool = pool_helpers.load_pool(path_to_pool(data_path, "pool_to_convert.json"))
        converted_pool = pool_helpers.convert_pool_to_ab_format(pool)
        assert converted_pool["data"]["metrics"]["abandonment rate"][0]["val"] is not None
        assert converted_pool["data"]["metrics"]["abandonment rate"][1]["val"] is not None
        assert converted_pool["data"]["metrics"]["abandonment rate"][1]["delta_prec"] is not None
        assert converted_pool["data"]["metrics"]["abandonment rate"][1]["pvalue"] is not None
        assert converted_pool["data"]["metrics"]["abandonment rate"][1]["delta_val"] is not None
        assert converted_pool["data"]["metrics"]["abandonment rate"][1].get("delta_percent") is not None

    def test_lost_elements(self, data_path):
        pool = pool_helpers.load_pool(path_to_pool(data_path, "pool_to_convert.json"))
        converted_pool = pool_helpers.convert_pool_to_ab_format(pool)
        assert len(converted_pool["data"]["metrics"]) == 1
        assert converted_pool["data"]["metrics"]["abandonment rate"] is not None
        assert len(converted_pool["data"]["testids"]) == 2
        assert converted_pool["data"]["testids"][0] == "143132"
        assert converted_pool["data"]["testids"][1] == "145566"

    def test_ab_info(self, data_path):
        pool = pool_helpers.load_pool(path_to_pool(data_path, "pool_to_convert_ab_info.json"))
        converted_pool = pool_helpers.convert_pool_to_ab_format(pool)
        assert converted_pool.get("meta") is not None
        assert converted_pool["meta"].get("groups") is not None
        assert len(converted_pool["meta"]["groups"]) == 3
        for group in converted_pool["meta"]["groups"]:
            assert group.get("metrics") is not None
            assert group.get("key") is not None
            if group["key"] == "group1":
                assert len(group["metrics"]) == 1
                assert group["metrics"][0].get("name") is not None
                assert group["metrics"][0]["name"] == "active users rate1"
                assert group["metrics"][0].get("controls_validated") is not None
                assert group["metrics"][0].get("releable") is not None
                assert group["metrics"][0].get("elite") is not None
                assert group["metrics"][0]["controls_validated"] and group["metrics"][0]["releable"] \
                       and group["metrics"][0]["elite"]
                assert group["metrics"][0].get("description") is not None
                assert group["metrics"][0]["description"] == "1"
                assert group["metrics"][0].get("hname") is None
            elif group["key"] == "group2":
                assert len(group["metrics"]) == 1
                assert group["metrics"][0].get("name") is not None
                assert group["metrics"][0]["name"] == "active users rate2"
                assert group["metrics"][0].get("controls_validated") is not None
                assert group["metrics"][0].get("releable") is not None
                assert group["metrics"][0].get("elite") is not None
                assert group["metrics"][0]["controls_validated"] and group["metrics"][0]["releable"] \
                       and not group["metrics"][0]["elite"]
                assert group["metrics"][0].get("description") is not None
                assert group["metrics"][0]["description"] == "2 version"
                assert group["metrics"][0].get("hname") is not None
                assert group["metrics"][0]["hname"] == "user rate num 2"
            elif group["key"] == "non-grouped metrics":
                assert len(group["metrics"]) == 2
                assert group["metrics"][0].get("name") is not None
                assert group["metrics"][0].get("controls_validated") is not None
                assert group["metrics"][0].get("releable") is not None
                assert group["metrics"][0].get("elite") is not None
                assert group["metrics"][1].get("name") is not None
                assert group["metrics"][1].get("controls_validated") is not None
                assert group["metrics"][1].get("releable") is not None
                assert group["metrics"][1].get("elite") is not None

                for metric in group["metrics"]:
                    if metric["name"] == "active users rate3":
                        assert metric["controls_validated"] and not metric["releable"] and not metric["elite"]
                        assert metric.get("description") is not None
                        assert metric["description"] == "3"
                    elif metric["name"] == "active users rate4":
                        assert (not metric["controls_validated"]) and not metric["releable"] and not metric["elite"]
                        assert metric.get("description") is None
                        assert metric.get("hname") is None
                    else:
                        raise Exception("Non-valid metric name")
            else:
                raise Exception("Non-valid group name")
