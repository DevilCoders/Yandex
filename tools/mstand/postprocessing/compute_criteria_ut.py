import logging

import postprocessing.compute_criteria as cc_pool
from experiment_pool import CriteriaResult  # noqa
from experiment_pool import MetricDataType
from experiment_pool import Pool
from postprocessing.compute_criteria_test_lib import (
    calc_criteria_result_on_observation,
    make_test_observation,
)


# noinspection PyClassHasNoInit
class TestComputeCriteria:
    data_size = 1000

    def test_criteria_very_different_samples(self, data_path, project_path):
        obs = make_test_observation("very_different_control.tsv", "very_different_exp.tsv",
                                    keyed=False, synthetic=False, data_path=data_path)
        cr_res, _ = calc_criteria_result_on_observation(obs, base_dir=project_path, synthetic=False)
        logging.info("criteria result: %s", cr_res)
        logging.info("criteria deviations: %s", cr_res.deviations)
        assert cr_res.pvalue < 1e-10
        assert cr_res.deviations.std_control > 0.1
        assert cr_res.deviations.std_exp > 0.1

    def test_criteria_almost_equal(self, data_path, project_path):
        obs = make_test_observation("almost_equal_control.tsv", "almost_equal_exp.tsv",
                                    keyed=False, synthetic=False, data_path=data_path)
        cr_res, _ = calc_criteria_result_on_observation(obs, base_dir=project_path, synthetic=False)
        logging.info("criteria result: %s", cr_res)
        logging.info("criteria deviations: %s", cr_res.deviations)
        assert cr_res.pvalue > 0.8
        assert cr_res.deviations.std_control > 0.1
        assert cr_res.deviations.std_exp > 0.1

    def test_criteria_keyed(self, data_path, project_path):
        obs = make_test_observation("keyed_control.tsv", "keyed_exp.tsv",
                                    keyed=True, synthetic=False, data_path=data_path)
        cr_res, _ = calc_criteria_result_on_observation(obs, base_dir=project_path, synthetic=False)
        logging.info("criteria result: %s", cr_res)
        logging.info("criteria deviations: %s", cr_res.deviations)
        assert cr_res.pvalue > 0.8
        assert cr_res.deviations.std_control > 0.1
        assert cr_res.deviations.std_exp > 0.1

    def test_synthetic_pool(self, data_path):
        obs = make_test_observation("keyed_control.tsv", "keyed_exp.tsv",
                                    keyed=True, synthetic=False, data_path=data_path)
        pool = Pool([obs])
        synth_pool = cc_pool.build_synthetic_pool(pool, MetricDataType.VALUES)
        assert synth_pool.observations
        for synth_obs in synth_pool.observations:
            assert synth_obs.id == obs.id
            assert synth_obs.control.metric_results
            assert not synth_obs.experiments

    def test_synthetic_pool_keyed(self, data_path):
        obs = make_test_observation("keyed_control.tsv", "keyed_exp.tsv",
                                    keyed=True, synthetic=False, data_path=data_path)
        pool = Pool([obs])
        synth_pool = cc_pool.build_synthetic_pool(pool, MetricDataType.KEY_VALUES)
        assert synth_pool.observations
        for synth_obs in synth_pool.observations:
            assert synth_obs.id == obs.id
            assert synth_obs.control.metric_results
            assert synth_obs.experiments
            assert all(exp.metric_results for exp in synth_obs.experiments)
