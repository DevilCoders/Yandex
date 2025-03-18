import logging

import numpy as np

from correlations import CorrelationPairForAPI
from correlations import CorrelationParamsForAPI
from correlations import CorrelationValuesForAPI
from correlations import MetricValuesForAPI
from correlations.chi_squared import ChiSquared
from experiment_pool import MetricValues


class MetricDiffDummy:
    def __init__(self, rel_diff, abs_diff):
        self.rel_diff = rel_diff
        self.abs_diff = abs_diff


class DeviationsDummy:
    def __init__(self, std_diff):
        self.std_diff = std_diff


# noinspection PyClassHasNoInit
class TestChiSquared:
    def test_chi_squared(self):
        sample_sz = 10 ** 6
        n_bins = 1000
        n_degrees_of_freedom = 3

        # noinspection PyArgumentList
        samp_x = np.random.randn(sample_sz)
        # noinspection PyArgumentList
        samp_y = np.random.randn(sample_sz)
        p_x = 1.0 / n_bins * np.arange(0, n_bins + 1)
        coef_start = -2.0
        coef_end = 2.1
        coef_step = 0.4
        for coef in np.arange(coef_start, coef_end, coef_step):
            logging.info("coef = %1.3f, percentage: %1.2f", coef, 100.0 * (coef - coef_start) / (coef_end - coef_start))
            vect_x = 5 * samp_x
            y_vect = coef * vect_x + 0.01 * samp_y
            params = []
            for i in range(p_x.size - 1):
                idx = np.logical_and(vect_x >= p_x[i], vect_x < p_x[i + 1])

                mean_x_s = np.mean(vect_x[idx])
                mean_y_s = np.mean(y_vect[idx])
                d_x_s = np.std(vect_x[idx]) / np.sqrt(np.sum(idx))
                d_y_s = np.std(y_vect[idx]) / np.sqrt(np.sum(idx))

                control_values = MetricValuesForAPI(MetricValues(1, 1, 1, 'values'))
                exp_values = MetricValuesForAPI(MetricValues(1, 1, 1, 'values'))
                metric_diff = MetricDiffDummy(mean_x_s, mean_x_s)
                pvalue = 0
                deviations = DeviationsDummy(d_x_s)
                # noinspection PyTypeChecker
                left = CorrelationValuesForAPI(control_values, exp_values, metric_diff, pvalue, deviations)

                control_values = MetricValuesForAPI(MetricValues(1, 1, 1, 'values'))
                exp_values = MetricValuesForAPI(MetricValues(1, 1, 1, 'values'))
                metric_diff = MetricDiffDummy(mean_y_s, mean_y_s)
                pvalue = 0
                deviations = DeviationsDummy(d_y_s)
                # noinspection PyTypeChecker
                right = CorrelationValuesForAPI(control_values, exp_values, metric_diff, pvalue, deviations)

                pair = CorrelationPairForAPI(left, right)
                params.append(pair)

            chi_params = CorrelationParamsForAPI(params)

            chi_sqr_runner = ChiSquared(n_degrees_of_freedom, use_relative_diff=True, use_std_left=True)
            chi_sqr = chi_sqr_runner(chi_params)
            assert chi_sqr < 1.25
            assert chi_sqr > 0.85

            chi_sqr_runner = ChiSquared(n_degrees_of_freedom, use_relative_diff=True, use_std_left=False)
            chi_sqr = chi_sqr_runner(chi_params)
            assert chi_sqr < 1.2
            assert chi_sqr > 0.85

            chi_sqr_runner = ChiSquared(n_degrees_of_freedom, use_relative_diff=False, use_std_left=True)
            chi_sqr = chi_sqr_runner(chi_params)
            assert chi_sqr < 1.2
            assert chi_sqr > 0.85

            chi_sqr_runner = ChiSquared(n_degrees_of_freedom, use_relative_diff=False, use_std_left=False)
            chi_sqr = chi_sqr_runner(chi_params)
            assert chi_sqr < 1.2
            assert chi_sqr > 0.85

            chi_sqr_runner = ChiSquared(n_degrees_of_freedom, use_relative_diff=False,
                                        use_std_left=False, use_prediction_offset=False)
            chi_sqr = chi_sqr_runner(chi_params)
            assert chi_sqr < 1.2
            assert chi_sqr > 0.85

            chi_sqr_runner = ChiSquared(n_degrees_of_freedom, t_statistic_threshold=5,
                                        use_relative_diff=False, use_std_left=False)
            chi_sqr = chi_sqr_runner(chi_params)
            assert chi_sqr < 1.3
            assert chi_sqr > 0.8

            chi_sqr_runner = ChiSquared(n_degrees_of_freedom, t_statistic_threshold=0.1,
                                        use_relative_diff=False, use_std_left=False)
            chi_sqr = chi_sqr_runner(chi_params)
            assert chi_sqr > 0.0
