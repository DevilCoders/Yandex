import logging
import os

import postprocessing.compute_criteria as cc_pool
import postprocessing.compute_criteria_single as cc_single
import yaqutils.time_helpers as utime
from criterias import TTest
from experiment_pool import CriteriaResult  # noqa
from experiment_pool import Experiment
from experiment_pool import MetricDataType
from experiment_pool import MetricResult
from experiment_pool import MetricType
from experiment_pool import MetricValues
from experiment_pool import Observation
from experiment_pool import Pool
from postprocessing import CriteriaContext
from postprocessing.criteria_read_mode import CriteriaReadMode
from user_plugins import PluginKey
from yaqutils import DateRange


def sample_path(data_path, file_name):
    return os.path.join(data_path, "compute_criteria", file_name)


# also used in compute_criteria_synthetic_ut_fat.py
def make_test_observation(control_file, exp_file, keyed, synthetic, data_path):
    """
    :type control_file: str
    :type exp_file: str | None
    :type keyed: bool
    :type synthetic: bool
    :rtype: Observation
    """
    metric_key = PluginKey("TestMetric")
    control = Experiment(testid="1")

    data_type = MetricDataType.KEY_VALUES if keyed else MetricDataType.VALUES
    metric_type = MetricType.OFFLINE if keyed else MetricType.ONLINE

    control_data_file = sample_path(data_path, control_file)
    control_metric_values = MetricValues(count_val=None,
                                         average_val=None,
                                         sum_val=None,
                                         data_type=data_type,
                                         data_file=control_data_file)
    control_metric_result = MetricResult(metric_values=control_metric_values,
                                         metric_type=metric_type,
                                         metric_key=metric_key)

    control.add_metric_result(control_metric_result)

    experiments = []

    if exp_file:
        exp1 = Experiment(testid="2")
        exp_data_file = sample_path(data_path, exp_file)

        exp_metric_values = MetricValues(count_val=None,
                                         average_val=None,
                                         sum_val=None,
                                         data_type=data_type,
                                         data_file=exp_data_file)
        exp_metric_result = MetricResult(metric_values=exp_metric_values,
                                         metric_type=metric_type,
                                         metric_key=metric_key)

        exp1.add_metric_result(exp_metric_result)
        experiments.append(exp1)

        if not synthetic:
            exp2 = Experiment(testid="3")
            exp_data_file = sample_path(data_path, exp_file)

            exp_metric_values = MetricValues(count_val=None,
                                             average_val=None,
                                             sum_val=None,
                                             data_type=data_type,
                                             data_file=exp_data_file)
            exp_metric_result = MetricResult(metric_values=exp_metric_values,
                                             metric_type=metric_type,
                                             metric_key=metric_key)

            exp2.add_metric_result(exp_metric_result)

            experiments.append(exp2)

    dates = DateRange(utime.parse_date_msk("20170101"), utime.parse_date_msk("20170101"))
    return Observation(obs_id=None, dates=dates, control=control, experiments=experiments)


def calc_criteria_result_on_observation(obs, synthetic, base_dir, flatten_mode=False):
    """
    :type obs: Observation
    :type synthetic: bool
    :type base_dir: str
    :type flatten_mode: bool
    :rtype: CriteriaResult | None
    """
    logging.info("flatten = %s", flatten_mode)
    criteria = TTest(flatten_mode=flatten_mode)
    criteria_key = PluginKey("CriteriaNameOne")

    synth_count = 5000 if synthetic else None

    criteria_read_mode = CriteriaReadMode.from_criteria(criteria)
    data_type = obs.control.metric_results[0].metric_values.data_type

    cr_arg = CriteriaContext(criteria=criteria,
                             criteria_key=criteria_key,
                             observation=obs,
                             data_type=data_type,
                             base_dir=base_dir,
                             read_mode=criteria_read_mode,
                             synth_count=synth_count)

    if synthetic:
        obs = cc_single.calc_synthetic_criteria_on_observation(cr_arg)
    else:
        obs = cc_single.calc_criteria_on_observation(cr_arg)

    # ... and now, run same data on pool.
    pool = Pool([obs])
    criteria_key_new = PluginKey("CriteriaNameTwo")
    cc_pool.calc_criteria(criteria=criteria,
                          criteria_key=criteria_key_new,
                          pool=pool,
                          base_dir=base_dir,
                          data_type=data_type)

    if not synthetic:
        assert_criteria_extra_data(pool)

    if synthetic:
        synth_res = cc_pool.calc_synthetic_summaries(pool.observations, data_type)
        cr_res = None
    else:
        cr_res = obs.experiments[0].metric_results[0].criteria_results[0]
        synth_res = None

    return cr_res, synth_res


def assert_criteria_extra_data(pool):
    """
    :type pool: Pool
    """
    assert pool.observations
    for obs in pool.observations:
        assert obs.experiments
        for exp in obs.experiments:
            assert exp.metric_results
            for m_res in exp.metric_results:
                assert m_res.criteria_results
                for c_res in m_res.criteria_results:
                    assert c_res.extra_data
