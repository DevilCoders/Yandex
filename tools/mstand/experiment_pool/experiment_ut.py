import datetime

import pytest

from experiment_pool import ExpErrorType
from experiment_pool import Experiment
from experiment_pool import MetricDataType
from experiment_pool import MetricResult
from experiment_pool import MetricType
from experiment_pool import MetricValues
from user_plugins import PluginKey

jan1_2016 = datetime.date(2016, 1, 1)
jan2_2016 = datetime.date(2016, 1, 2)
jan1_2015 = datetime.date(2015, 1, 1)


# noinspection PyClassHasNoInit
class TestExperimentOperators:
    def test_equal(self):
        exp = Experiment(testid="111")
        exp_same = Experiment(testid="111")
        exp_other = Experiment(testid="222")

        with pytest.raises(Exception):
            assert (exp == exp_same)

        with pytest.raises(Exception):
            assert (exp != exp_other)

        with pytest.raises(Exception):
            return {exp: None}

    def test_serialize(self):
        exp = Experiment(testid="123", serpset_id="100500")
        seralized = {
            "testid": "123",
            "serpset_id": "100500"
        }
        assert seralized == exp.serialize()

    def test_all_testids(self):
        exp = Experiment(testid="100500")
        assert exp.all_testids() == ("100500",)

    def test_all_testids_null(self):
        exp = Experiment(testid=None, serpset_id="100500")
        assert exp.all_testids() == tuple()

    def test_all_serpset_ids(self):
        exp = Experiment(testid="100500", serpset_id="78901234")
        assert exp.all_serpset_ids() == ("78901234",)

    def test_all_serpset_ids_null(self):
        exp = Experiment(testid="9876", serpset_id=None)
        assert exp.all_serpset_ids() == tuple()

    def test_all_testids_with_errors(self):
        exp = Experiment(testid="9876", serpset_id="123456", errors=[ExpErrorType.SERP_FETCH])
        assert exp.all_testids() == tuple()
        assert exp.all_testids(include_errors=True) == ("9876",)

    def test_all_serpset_ids_with_errors(self):
        exp = Experiment(testid="9876", serpset_id="123456", errors=[ExpErrorType.SERP_FETCH])
        assert exp.all_serpset_ids() == tuple()
        assert exp.all_serpset_ids(include_errors=True) == ("123456",)

    def test_repr(self):
        exp = Experiment(testid="100500")
        exp_repr = "{}".format(exp)
        assert ("100500" in exp_repr)

    def test_hash(self):
        exp = Experiment(testid="100500")
        with pytest.raises(Exception):
            return {exp: None}

    def test_add_metric_result(self):
        exp = Experiment(testid="100500")
        metric_values = MetricValues(count_val=None, average_val=0.5, sum_val=0,
                                     data_type=MetricDataType.VALUES, data_file="qqq.tsv")
        metric_key = PluginKey(name="test_metric")
        mr = MetricResult(metric_key=metric_key, metric_values=metric_values, metric_type=MetricType.ONLINE)
        metric_key_other = PluginKey(name="test_metric_other")
        mr_other = MetricResult(metric_key=metric_key_other, metric_values=metric_values, metric_type=MetricType.ONLINE)
        mr_dup = MetricResult(metric_key=metric_key, metric_values=metric_values, metric_type=MetricType.ONLINE)
        exp.add_metric_result(mr)
        exp.add_metric_result(mr_other)
        with pytest.raises(Exception):
            exp.add_metric_result(mr_dup)

        assert exp.all_data_types() == {MetricDataType.VALUES}

    def test_metric_result_map(self):
        exp = Experiment(testid="100500")
        metric_values = MetricValues(count_val=None, average_val=0.5, sum_val=0, data_type=MetricDataType.VALUES)
        metric_key1 = PluginKey(name="test_metric1")
        metric_key2 = PluginKey(name="test_metric2")
        mr1 = MetricResult(metric_key=metric_key1, metric_values=metric_values, metric_type=MetricType.ONLINE)
        mr2 = MetricResult(metric_key=metric_key2, metric_values=metric_values, metric_type=MetricType.ONLINE)
        exp.add_metric_result(mr1)
        exp.add_metric_result(mr2)

        metric_results_map = exp.get_metric_results_map()
        assert metric_results_map[metric_key1].key() == mr1.key()
        assert metric_results_map[metric_key2].key() == mr2.key()

    def test_experiment_clone(self):
        exp = Experiment(testid="100500")

        metric_values = MetricValues(count_val=None, average_val=0.5, sum_val=0, data_type=MetricDataType.VALUES)
        metric_key = PluginKey(name="test_metric2")
        mr = MetricResult(metric_key=metric_key, metric_values=metric_values, metric_type=MetricType.ONLINE)
        exp.add_metric_result(mr)

        exp_clone = exp.clone()
        assert exp_clone is not exp
        assert not exp_clone.metric_results
