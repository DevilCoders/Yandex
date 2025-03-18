import pytest
import os
import yaqutils.json_helpers as ujson
import yaqutils.test_helpers as utest

import experiment_pool.pool_helpers as pool_helpers
from mstand_structs import squeeze_versions
from yaqutils import nirvana_helpers


def get_pool_path(path, name):
    return os.path.join(path, name)


def try_load_pool(path, name):
    return pool_helpers.load_pool(get_pool_path(path, name))


def compare_pool(path, pool, expected):
    serialized = pool.serialize()
    expected_path = get_pool_path(path, expected)
    expected_data = ujson.load_from_file(expected_path)
    utest.diff_structures(serialized, expected_data)


def load_and_compare(path, name, expected):
    pool = try_load_pool(path, name)
    compare_pool(path, pool, expected)


def load_and_merge(path, names):
    return pool_helpers.load_and_merge_pools([get_pool_path(path, name) for name in names])


# noinspection PyClassHasNoInit
class TestDeserializerV0:
    def test_v0_basic(self, data_path):
        load_and_compare(data_path, "v0/basic_input.json", "v1/basic_input.json")

    def test_v0_long(self, data_path):
        load_and_compare(data_path, "v0/basic_long.json", "v1/basic_long.json")

    def test_v0_repeated_observations(self, data_path):
        pool = try_load_pool(data_path, "v0/repeated_observations.json")
        assert len(pool.observations) == 1
        control = pool.observations[0].control
        assert len(control.metric_results) == 2
        first_exp = pool.observations[0].experiments[0]
        assert len(first_exp.metric_results) == 2

    def test_v0_empty(self, data_path):
        with pytest.raises(Exception):
            try_load_pool(data_path, "v0/empty.json")


# noinspection PyClassHasNoInit
class TestDeserializerV1:
    def test_v1_simple(self, data_path):
        load_and_compare(data_path, "v1/simple_pool.json", "v1/simple_pool.json")

    def test_v1_with_criterias(self, data_path):
        load_and_compare(data_path, "v1/pool_with_criteria_results.json", "v1/pool_with_criteria_results.json")

    def test_v1_abt(self, data_path):
        load_and_compare(data_path, "v1/pool_abt.json", "v1/pool_abt.json")

    def test_v1_same_metric_twice(self, data_path):
        try_load_pool(data_path, "v1/same_metric_twice_different_exps.json")
        with pytest.raises(Exception):
            try_load_pool(data_path, "v1/same_metric_twice.json")

    def test_v1_minimal(self, data_path):
        try_load_pool(data_path, "v1/very_minimal_pool.json")

    def test_v1_lamps(self, data_path, monkeypatch):
        monkeypatch.setattr(squeeze_versions, "get_python_version", lambda: None)
        monkeypatch.setattr(squeeze_versions, "get_revision", lambda: None)
        monkeypatch.setattr(nirvana_helpers, "get_nirvana_workflow_url", lambda: None)
        load_and_compare(data_path, "v1/pool_with_lamps.json", "v1/pool_with_lamps.json")

    def test_v1_merge_pool(self, data_path):
        pool = load_and_merge(data_path, [
            "v1/merge_base.json",
            "v1/merge_pool.json"
        ])
        assert len(pool.observations) == 2
        assert pool.observations[1].id == "9"
        assert pool.observations[0].id == "12345"

        assert pool.observations[1].control.testid == "12"
        assert len(pool.observations[1].control.metric_results) == 1

        first_exp = pool.observations[1].experiments[0]
        assert first_exp.testid == "34"
        assert len(first_exp.metric_results) == 1
        assert len(first_exp.metric_results[0].criteria_results) == 1

        assert pool.observations[0].control.testid == "1234"
        assert len(pool.observations[0].experiments) == 0

        assert pool.extra_data == {"f1": "v1", "f2": "v2"}

    def test_v1_merge_experiment(self, data_path):
        pool = load_and_merge(data_path, [
            "v1/merge_base.json",
            "v1/merge_experiment.json"
        ])
        assert len(pool.observations) == 1
        obs = pool.observations[0]
        assert len(obs.experiments) == 1
        control = obs.control
        experiment = obs.experiments[0]
        assert len(control.metric_results) == 2
        assert len(experiment.metric_results) == 2
        assert control.metric_results[0].metric_key.name == "metric_one"
        assert control.metric_results[1].metric_key.name == "metric_two"
        assert experiment.metric_results[0].metric_key.name == "metric_one"
        assert experiment.metric_results[1].metric_key.name == "metric_two"

    def test_v1_merge_metric_result(self, data_path):
        pool = load_and_merge(data_path, [
            "v1/merge_base.json",
            "v1/merge_metric_result.json"
        ])
        assert len(pool.observations) == 1
        obs = pool.observations[0]
        assert len(obs.experiments) == 1
        experiment = obs.experiments[0]
        assert len(experiment.metric_results) == 1
        metric_result = experiment.metric_results[0]
        assert len(metric_result.criteria_results) == 2
        assert metric_result.criteria_results[1].criteria_key.name == "criteria"
        assert metric_result.criteria_results[0].criteria_key.name == "another_criteria"
