import logging

import scipy.stats

from experiment_pool import CriteriaResult
from experiment_pool import Deviations
from experiment_pool import MetricDataType
from experiment_pool import MetricResult
from experiment_pool import MetricType
from experiment_pool import MetricValueType
from experiment_pool import MetricValues
from experiment_pool import MetricColoring
from user_plugins import PluginKey

import math

EPS = 1e-10


def _create_metric_result(name, value):
    return MetricResult(
        PluginKey(name),
        MetricType.ONLINE,
        MetricValues(
            count_val=1,
            sum_val=value,
            average_val=value,
            data_type=MetricDataType.NONE,
            value_type=MetricValueType.AVERAGE
        )
    )


def _build_result_name_map(experiment):
    results = {}
    for key, result in experiment.get_metric_results_map().items():
        results[key.pretty_name()] = result
    return results


def _build_value_map(experiment):
    values = {}
    for key, result in experiment.get_metric_results_map().items():
        values[key.pretty_name()] = result.metric_values.significant_value
    return values


def set_metric_sign(metric_coloring, result_coloring):
    if metric_coloring == result_coloring:
        return 1.0
    else:
        return -1.0


def is_values_equal(value1, value2):
    return abs(value1 - value2) < EPS


class CombineMetricValues(object):
    def __init__(self, expr, name="combined_metric"):
        self.expr = expr
        self.name = name

    def process_pool(self, pool_api):
        compiled_expr = compile(self.expr, "<string>", "eval")
        for exp in pool_api.pool.all_experiments():
            logging.info("Processing experiment %s...", exp)
            values = _build_value_map(exp)
            new_value = eval(compiled_expr, {"v": values, "math": math}, {})
            exp.add_metric_result(_create_metric_result(self.name, new_value))
        return pool_api


class WeightedSum(object):
    def __init__(self, name1, name2, weight1, weight2, remove_others=False, coloring=MetricColoring.MORE_IS_BETTER):
        assert name1 is not None, "Metric1's name does not set"
        assert name2 is not None, "Metric2's name does not set"
        assert weight1 is not None, "Metric1's weight does not set"
        assert weight2 is not None, "Metric2's weight does not set"

        self.name1 = name1
        self.name2 = name2
        self.weight1 = weight1
        self.weight2 = weight2
        self.remove_others = remove_others
        self.coloring = coloring

        self._result_name = "{}_{}_{}_{}".format(weight1, name1, weight2, name2)

    def add_metric_result(self, exp, m_res):
        if self.remove_others:
            exp.metric_results = [m_res]
        else:
            exp.add_metric_result(m_res)

    def process_pool(self, pool_api):
        weight1 = self.weight1
        weight2 = self.weight2

        for obs in pool_api.pool.observations:
            logging.info("Processing observation %s...", obs)

            assert len(obs.experiments) > 0, "No experiments in observation {}, only control.".format(obs)

            exp_values = _build_result_name_map(obs.experiments[0])

            exp_res1 = exp_values[self.name1]
            exp_res2 = exp_values[self.name2]

            crit_res1 = exp_res1.criteria_results[0]
            crit_res2 = exp_res2.criteria_results[0]

            std_control_norm1 = crit_res1.deviations.std_control
            std_control_norm2 = crit_res2.deviations.std_control

            assert not is_values_equal(std_control_norm1, 0.0)
            assert not is_values_equal(std_control_norm2, 0.0)

            control_results = _build_result_name_map(obs.control)

            metric1_sign = set_metric_sign(control_results[self.name1].coloring, self.coloring)
            metric2_sign = set_metric_sign(control_results[self.name2].coloring, self.coloring)

            control_v1 = control_results[self.name1].metric_values.significant_value
            control_v2 = control_results[self.name2].metric_values.significant_value

            control_new_value = (control_v1 * float(weight1) * metric1_sign / std_control_norm1) + \
                                (control_v2 * float(weight2) * metric2_sign / std_control_norm2)

            self.add_metric_result(obs.control, _create_metric_result(self._result_name, control_new_value))

            for exp in obs.experiments:
                logging.info("Processing experiment %s vs control %s...", exp, obs.control)

                exp_values = _build_result_name_map(exp)
                exp_res1 = exp_values[self.name1]
                exp_res2 = exp_values[self.name2]

                exp_v1 = exp_res1.metric_values.significant_value
                exp_v2 = exp_res2.metric_values.significant_value

                crit_res1 = exp_res1.criteria_results[0]
                crit_res2 = exp_res2.criteria_results[0]

                std_control1 = crit_res1.deviations.std_control
                std_control2 = crit_res2.deviations.std_control

                assert is_values_equal(std_control1, std_control_norm1)
                assert is_values_equal(std_control2, std_control_norm2)
                assert not is_values_equal(std_control1, 0.0)
                assert not is_values_equal(std_control2, 0.0)

                std_exp1 = crit_res1.deviations.std_exp
                std_exp2 = crit_res2.deviations.std_exp

                std_diff1 = crit_res1.deviations.std_diff
                std_diff2 = crit_res2.deviations.std_diff

                new_value = (exp_v1 * float(weight1) * metric1_sign / std_control_norm1) + (exp_v2 * float(weight2) * metric2_sign / std_control_norm2)

                new_std_control = math.sqrt((weight1 * std_control1 / std_control_norm1) ** 2 + (weight2 * std_control2 / std_control_norm2) ** 2)
                new_std_exp = math.sqrt((weight1 * std_exp1 / std_control_norm1) ** 2 + (weight2 * std_exp2 / std_control_norm2) ** 2)
                new_std_diff = math.sqrt((weight1 * std_diff1 / std_control_norm1) ** 2 + (weight2 * std_diff2 / std_control_norm2) ** 2)

                t_stat = (new_value - control_new_value) / new_std_diff
                pvalue = 2 * (1 - scipy.stats.norm.cdf(abs(t_stat)))

                new_result = _create_metric_result(self._result_name, new_value)
                new_result.add_criteria_result(
                    CriteriaResult(
                        PluginKey("fake-ttest"),
                        pvalue=pvalue,
                        deviations=Deviations(
                            std_control=new_std_control,
                            std_exp=new_std_exp,
                            std_diff=new_std_diff
                        )
                    )
                )

                self.add_metric_result(exp, new_result)

        return pool_api
