import pytest

import yaqutils.json_helpers as ujson

from user_plugins import PluginKey
from experiment_pool import Deviations
from experiment_pool import CriteriaResult


# noinspection PyClassHasNoInit
class TestCriteriaResult:
    def test_serialize_criteria_result(self):
        criteria_key = PluginKey(name="criteria_name")
        criteria_result = CriteriaResult(criteria_key=criteria_key, pvalue=0.65)

        serialized_exp = {
            "criteria_key": {
                "name": "criteria_name"
            },
            "pvalue": 0.65
        }

        serialized_act = criteria_result.serialize()
        assert serialized_act == serialized_exp
        ujson.dump_to_str(serialized_act)

    def test_serialize_criteria_result_with_nan(self):
        criteria_key = PluginKey(name="criteria_name")
        nan = float('nan')
        criteria_result = CriteriaResult(criteria_key=criteria_key, pvalue=nan)

        # MSTAND-943
        serialized_exp = {
            "criteria_key": {
                "name": "criteria_name"
            },
            "pvalue": None
        }
        serialized_act = criteria_result.serialize()
        assert serialized_act == serialized_exp
        ujson.dump_to_str(serialized_act)

    def test_serialize_criteria_result_with_none(self):
        criteria_key = PluginKey(name="criteria_name")
        criteria_result = CriteriaResult(criteria_key=criteria_key, pvalue=None)
        ujson.dump_to_str(criteria_result.serialize())

    def test_serialize_criteria_result_with_deviations(self):
        criteria_key = PluginKey(name="criteria_name")
        deviations = Deviations(0.01, 0.02, 0.03)
        criteria_result = CriteriaResult(criteria_key=criteria_key, pvalue=0.65, deviations=deviations)

        serialized_exp = {
            "criteria_key": {
                "name": "criteria_name"
            },
            "pvalue": 0.65,
            "deviations": {
                "std_control": 0.01,
                "std_exp": 0.02,
                "std_diff": 0.03
            }
        }
        serialized_act = criteria_result.serialize()
        assert serialized_exp == serialized_act
        CriteriaResult.deserialize(serialized_act)

    def test_deserialize_criteria_with_none_pvalue(self):
        serialized = {
            "criteria_key": {
                "name": "criteria_name"
            },
            "pvalue": None
        }
        CriteriaResult.deserialize(serialized)

    def test_compare_criteria_result(self):
        criteria_key1 = PluginKey(name="criteria_name1")
        criteria_key2 = PluginKey(name="criteria_name2")
        crit_res1 = CriteriaResult(criteria_key=criteria_key1, pvalue=0.65)
        crit_res2 = CriteriaResult(criteria_key=criteria_key2, pvalue=0.65)
        with pytest.raises(Exception):
            return crit_res1 == crit_res2

    def test_hash(self):
        criteria_key = PluginKey(name="criteria_name")
        crit_res = CriteriaResult(criteria_key=criteria_key, pvalue=0.65)
        with pytest.raises(Exception):
            return {crit_res: None}

    def test_serialize_deviations(self):
        deviations = Deviations(std_control=float('nan'), std_exp=None, std_diff=1)
        serialized = deviations.serialize()
        assert serialized["std_control"] is None
        assert serialized["std_diff"] == 1

    def test_deserialize_deviations(self):
        deviations_data = {
            "std_control": 0.01,
            "std_exp": 0.02,
            "std_diff": 0.03
        }
        dev = Deviations.deserialize(deviations_data)
        assert dev.std_control == 0.01
        assert dev.std_exp == 0.02
        assert dev.std_diff == 0.03
