import pytest
import datetime

from experiment_pool import CriteriaResult
from experiment_pool import MetricDiff
from experiment_pool import MetricResult
from experiment_pool import LampResult
from experiment_pool import MetricValues
from experiment_pool import MetricColoring
from experiment_pool import MetricType
from experiment_pool import MetricDataType
from experiment_pool import MetricValueDiff
from mstand_structs import squeeze_versions
from user_plugins import PluginKey
from user_plugins import PluginABInfo

from mstand_structs import LampKey
from mstand_structs import SqueezeVersions
from yaqutils import DateRange
from yaqutils import nirvana_helpers


# noinspection PyClassHasNoInit
class TestMetricResult:
    def test_serialize_metric_result(self):
        metric_key = PluginKey(name="metric_name")
        metric_values = MetricValues(count_val=10, average_val=0.75, sum_val=12, data_type=MetricDataType.VALUES)
        ab_info = PluginABInfo("group", "elite", "description", "hname")
        metric_result = MetricResult(metric_key=metric_key,
                                     metric_type=MetricType.ONLINE,
                                     metric_values=metric_values,
                                     coloring=MetricColoring.MORE_IS_BETTER,
                                     ab_info=ab_info)

        serialized = {
            "metric_key": {
                "name": "metric_name"
            },
            "values": {
                "average": 0.75,
                "sum": 12,
                "count": 10,
                "data_type": "values",
                "value_type": "average"
            },
            "type": "online",
            "coloring": "more-is-better",
            "ab_info": {
                "group": "group",
                "star": "elite",
                "description": "description",
                "hname": "hname"
            }
        }
        assert serialized == metric_result.serialize()
        MetricResult.deserialize(serialized)

    def test_serialize_metric_result_with_synthetic_criterias(self):
        crit_key1 = PluginKey(name="criteria1")
        crit_key2 = PluginKey(name="criteria2")

        crit_res1 = CriteriaResult(criteria_key=crit_key1, pvalue=0.75, synthetic=False)
        crit_res2 = CriteriaResult(criteria_key=crit_key2, pvalue=0.36, synthetic=True)

        metric_key = PluginKey(name="metric_name")
        metric_values = MetricValues(count_val=10, average_val=0.6543, sum_val=12,
                                     data_type=MetricDataType.VALUES, row_count=11)
        ab_info = PluginABInfo("group", "elite", "description", "hname")

        metric_result = MetricResult(metric_key=metric_key,
                                     metric_type=MetricType.ONLINE,
                                     metric_values=metric_values,
                                     criteria_results=[crit_res1, crit_res2],
                                     ab_info=ab_info)

        serialized = {
            "metric_key": {
                "name": "metric_name"
            },
            "values": {
                "average": 0.6543,
                "sum": 12,
                "count": 10,
                "row_count": 11,
                "data_type": "values",
                "value_type": "average"
            },
            "type": "online",
            "criterias": [
                {
                    "criteria_key": {
                        "name": "criteria1"
                    },
                    "pvalue": 0.75
                }
            ],
            "synthetic_criterias": [
                0.36
            ],
            "ab_info": {
                "group": "group",
                "star": "elite",
                "description": "description",
                "hname": "hname"
            }
        }
        assert serialized == metric_result.serialize()
        deserialized = MetricResult.deserialize(serialized)

        assert "0.6543" in str(metric_result)
        assert not deserialized.version

    def test_result_with_version_serialize(self, monkeypatch):
        monkeypatch.setattr(squeeze_versions, "get_python_version", lambda: None)
        monkeypatch.setattr(squeeze_versions, "get_revision", lambda: None)
        monkeypatch.setattr(nirvana_helpers, "get_nirvana_workflow_url", lambda: None)

        version = SqueezeVersions({"web": 1}, 1, 2)
        metric_key = PluginKey(name="metric_name")
        metric_values = MetricValues(count_val=10, average_val=0.6543, sum_val=12,
                                     data_type=MetricDataType.VALUES, row_count=11)

        metric_result = MetricResult(metric_key=metric_key,
                                     metric_type=MetricType.ONLINE,
                                     metric_values=metric_values,
                                     version=version)

        serialized = {
            "metric_key": {
                "name": "metric_name"
            },
            "values": {
                "average": 0.6543,
                "sum": 12,
                "count": 10,
                "row_count": 11,
                "data_type": "values",
                "value_type": "average"
            },
            "type": "online",
            "version": {
                "web": 1,
                "_common": 1,
                "_history": 2
            }
        }

        assert serialized == metric_result.serialize()
        deserialized = MetricResult.deserialize(serialized)
        assert repr(deserialized.version) == repr(version)

    def test_criteria_results_map(self):
        metric_key = PluginKey(name="metric_name")
        metric_values = MetricValues(count_val=10, average_val=0.75, sum_val=12, data_type=MetricDataType.VALUES)
        crit_key1 = PluginKey(name="criteria1")
        crit_key2 = PluginKey(name="criteria2")
        crit_res1 = CriteriaResult(criteria_key=crit_key1, pvalue=0.75)
        crit_res2 = CriteriaResult(criteria_key=crit_key2, pvalue=0.36)
        metric_result = MetricResult(metric_key=metric_key,
                                     metric_type=MetricType.ONLINE,
                                     metric_values=metric_values,
                                     criteria_results=[crit_res1, crit_res2])
        crit_res_map = metric_result.get_criteria_results_map()
        assert len(crit_res_map) == 2
        assert crit_res_map[crit_key1].key() == crit_res1.key()
        assert crit_res_map[crit_key2].key() == crit_res2.key()


# noinspection PyClassHasNoInit
class TestMetricValues:
    def test_serialize_metric_values(self):
        metric_values = MetricValues(count_val=10, average_val=0.75, sum_val=12, data_type=MetricDataType.VALUES)

        serialized = {
            "average": 0.75,
            "sum": 12,
            "count": 10,
            "data_type": "values",
            "value_type": "average"
        }
        assert serialized == metric_values.serialize()

    def test_metric_values_operators(self):
        mv = MetricValues(count_val=100500, average_val=0.7531, sum_val=12, data_type=MetricDataType.VALUES)
        mv_other = MetricValues(count_val=11, average_val=0.93, sum_val=13, data_type=MetricDataType.VALUES)
        mv_same = MetricValues(count_val=100500, average_val=0.7531, sum_val=12, data_type=MetricDataType.VALUES)

        assert mv != mv_other
        assert mv == mv_same

        assert "0.7531" in str(mv)
        assert "100500" in str(mv)


# noinspection PyClassHasNoInit
class TestMetricDiff:
    def test_metric_diff_simple(self):
        mv = MetricValues(count_val=1, average_val=1, sum_val=1, data_type=MetricDataType.VALUES)
        mv_other = MetricValues(count_val=2, average_val=2, sum_val=3, data_type=MetricDataType.VALUES)

        diff = MetricDiff(mv, mv_other)

        assert diff.count.abs_diff == 1
        assert diff.count.rel_diff == 1
        assert diff.count.rel_diff_human == 1
        assert diff.count.perc_diff == 100
        assert diff.count.perc_diff_human == 100

        assert diff.average.abs_diff == 1
        assert diff.average.rel_diff == 1
        assert diff.average.rel_diff_human == 1
        assert diff.average.perc_diff == 100
        assert diff.average.perc_diff_human == 100

        assert diff.sum.abs_diff == 2
        assert diff.sum.rel_diff == 2
        assert diff.sum.rel_diff_human == 2
        assert diff.sum.perc_diff == 200
        assert diff.sum.perc_diff_human == 200

        diff_inv = MetricDiff(mv_other, mv)

        assert diff_inv.count.abs_diff == -1
        assert diff_inv.count.rel_diff == -0.5
        assert diff_inv.count.rel_diff_human == -0.5
        assert diff_inv.count.perc_diff == -50
        assert diff_inv.count.perc_diff_human == -50

        assert diff_inv.average.abs_diff == -1
        assert diff_inv.average.rel_diff == -0.5
        assert diff_inv.average.rel_diff_human == -0.5
        assert diff_inv.average.perc_diff == -50
        assert diff_inv.average.perc_diff_human == -50

        assert diff_inv.sum.abs_diff == -2
        assert abs(diff_inv.sum.rel_diff + 0.6666666) < 0.0001
        assert abs(diff_inv.sum.rel_diff_human + 0.6666666) < 0.0001
        assert abs(diff_inv.sum.perc_diff + 66.6666666) < 0.0001
        assert abs(diff_inv.sum.perc_diff_human + 66.6666666) < 0.0001

    def test_metric_diff_zero(self):
        vdiff = MetricValueDiff(0, 0)
        assert vdiff.rel_diff is None

    def test_metric_diff_very_small(self):
        vdiff = MetricValueDiff(1e-16, 1e-16)
        assert vdiff.rel_diff is None

    def test_metric_diff_negative(self):
        vdiff = MetricValueDiff(-10, -5)

        assert vdiff.abs_diff == 5
        assert vdiff.rel_diff == -0.5
        assert vdiff.rel_diff_human == 0.5
        assert vdiff.perc_diff == -50
        assert vdiff.perc_diff_human == 50


# noinspection PyClassHasNoInit
class TestLampResult(object):
    revision = 12345
    workflow_url = "workflow_url"

    dates = DateRange(datetime.date(2013, 10, 17), datetime.date(2013, 12, 23))
    version = SqueezeVersions({"web": 1}, 1, 2, revision=revision, workflow_url=workflow_url)
    lamp_key = LampKey(testid="5000", control="6000", observation="1", dates=dates, version=version)
    value_key = PluginKey(name="lampValue")
    lamp_val = MetricValues(count_val=10, average_val=0.75, sum_val=12, data_type=MetricDataType.VALUES)
    lamp_values = MetricResult(metric_key=value_key,
                               metric_type=MetricType.ONLINE,
                               metric_values=lamp_val,
                               coloring=MetricColoring.LAMP)
    lamp = LampResult(lamp_key=lamp_key, lamp_values=[lamp_values])

    def test_lamp_str(self):
        assert str(self.lamp_key) in str(self.lamp)
        assert str(self.lamp_values) in str(self.lamp)

    def test_lamp_operators(self):
        with pytest.raises(Exception):
            assert self.lamp == self.lamp

        with pytest.raises(Exception):
            hash(self.lamp)

    def test_lamp_serialization(self):
        serialized = {
            "lamp_key": {
                "testid": "5000",
                "control": "6000",
                "observation": "1",
                "dates": "20131017:20131223",
                "version": {
                    "web": 1,
                    "_common": 1,
                    "_history": 2,
                    "_python": squeeze_versions.get_python_version(),
                    "_revision": self.revision,
                    "_workflow_url": self.workflow_url,
                }
            },
            "coloring": "lamp",
            "values": [
                {
                    "metric_key": {
                        "name": "lampValue"
                    },
                    "values": {
                        "count": 10,
                        "average": 0.75,
                        "sum": 12,
                        "data_type": "values",
                        "value_type": "average"
                    },
                    "coloring": "lamp",
                    "type": "online"
                }
            ]
        }

        assert self.lamp.serialize() == serialized
