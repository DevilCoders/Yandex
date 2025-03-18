import logging

import numpy as np

from correlations import CorrelationParamsForAPI  # noqa


def validate_numbers(array, name):
    if any(x is None for x in array):
        raise Exception("Array {} contains None values: {}".format(name, array))


class ChiSquared(object):
    def __init__(self, n_degrees_of_freedom, t_statistic_threshold=None,
                 use_relative_diff=False, use_std_left=False, use_prediction_offset=True):
        self.n_degrees_of_freedom = n_degrees_of_freedom
        self.t_statistic_threshold = t_statistic_threshold
        self.use_relative_diff = use_relative_diff
        self.use_std_left = use_std_left
        self.use_prediction_offset = use_prediction_offset

    @staticmethod
    def make_rel_diff_data(params):
        # use relative diff, WARNING it works only for metrics with strong positive values
        metric_deltas_left = np.array([x.left.metric_diff.rel_diff for x in params.corr_pairs])
        metric_deltas_right = np.array([x.right.metric_diff.rel_diff for x in params.corr_pairs])

        validate_numbers(metric_deltas_left, "metric_deltas_left")
        validate_numbers(metric_deltas_right, "metric_deltas_right")

        left_deviations = [x.left.deviations for x in params.corr_pairs]
        if not all(left_deviations):
            logging.error("No deviations calculated for criteria on the left")
            return None

        right_deviations = [x.right.deviations for x in params.corr_pairs]

        if not all(right_deviations):
            logging.error("No deviations calculated for criteria on the right")
            return None

        std_diffs_left_unnorm = np.array([dev.std_diff for dev in left_deviations])
        std_diffs_right_unnorm = np.array([dev.std_diff for dev in right_deviations])

        validate_numbers(std_diffs_left_unnorm, "std_diffs_left_unnorm")
        validate_numbers(std_diffs_right_unnorm, "std_diffs_right_unnorm")

        abs_metric_control_left = np.array([x.left.control_values.value for x in params.corr_pairs])
        abs_metric_control_right = np.array([x.right.control_values.value for x in params.corr_pairs])

        if np.any(abs_metric_control_left <= 0):
            raise Exception("Zero value in left metric")  # maybe 'left' is not the best word
        if np.any(abs_metric_control_right <= 0):
            raise Exception("Zero value in right metric")  # maybe 'right' is not the best word

        std_diffs_left = std_diffs_left_unnorm / abs_metric_control_left
        std_diffs_right = std_diffs_right_unnorm / abs_metric_control_right

        return metric_deltas_left, metric_deltas_right, std_diffs_left, std_diffs_right

    @staticmethod
    def make_abs_diff_data(params):
        # use abs diff
        metric_deltas_left = np.array([x.left.metric_diff.abs_diff for x in params.corr_pairs])
        metric_deltas_right = np.array([x.right.metric_diff.abs_diff for x in params.corr_pairs])

        validate_numbers(metric_deltas_left, "metric_deltas_left")
        validate_numbers(metric_deltas_right, "metric_deltas_right")

        left_deviations = [x.left.deviations for x in params.corr_pairs]
        if not all(left_deviations):
            raise Exception("No deviations calculated for criteria on the left")

        right_deviations = [x.right.deviations for x in params.corr_pairs]

        if not all(right_deviations):
            raise Exception("No deviations calculated for criteria on the right")

        std_diffs_left = np.array([dev.std_diff for dev in left_deviations])
        std_diffs_right = np.array([dev.std_diff for dev in right_deviations])

        validate_numbers(std_diffs_left, "std_diffs_left")
        validate_numbers(std_diffs_right, "std_diffs_right")

        return metric_deltas_left, metric_deltas_right, std_diffs_left, std_diffs_right

    def __call__(self, params):
        """
        :type params: CorrelationParamsForAPI
        :rtype: float
        """
        # MSTAND-1669: hide sklearn import inside
        # Typically, correlation classes does not use sklearn.

        from sklearn.linear_model import LinearRegression

        if self.use_relative_diff:
            metric_deltas_left, metric_deltas_right, std_diffs_left, std_diffs_right = self.make_rel_diff_data(params)
        else:
            metric_deltas_left, metric_deltas_right, std_diffs_left, std_diffs_right = self.make_abs_diff_data(params)

        zeros = std_diffs_right == 0
        if np.any(zeros):
            logging.debug("zeros in diffs: %s", std_diffs_right)
            if np.all(zeros):
                raise Exception("All values in std_diffs_right are zeros, it's strange. ")
            logging.warning("Using mininum of std_diffs_right instead of zeros.")
            std_diffs_right[zeros] = np.min(std_diffs_right[np.logical_not(zeros)])

        # transposed matrix (n x 1) from vector (n)
        metric_deltas_left_col = np.array(metric_deltas_left, ndmin=2).T

        # iteratively select experiments with t_statistic < t_statistic_threshold
        t_statistic_selector = np.ones(len(metric_deltas_left), dtype=bool)

        while True:
            model = LinearRegression(fit_intercept=self.use_prediction_offset)
            model.fit(metric_deltas_left_col[t_statistic_selector, :], metric_deltas_right[t_statistic_selector])

            # noinspection PyArgumentList
            metric_deltas_right_predicted = model.predict(metric_deltas_left_col)
            diff_predict_truth = metric_deltas_right_predicted - metric_deltas_right

            # calculate t_statistic
            if self.use_std_left:
                # use std of left diffs, it is experimental option
                t_statistic = diff_predict_truth / np.sqrt(std_diffs_right ** 2 + (std_diffs_left * model.coef_) ** 2)
            else:
                # do not use std of left diffs
                t_statistic = diff_predict_truth / std_diffs_right

            # new selection of experiments
            if self.t_statistic_threshold is not None:
                next_t_statistic_selector = np.abs(t_statistic) < self.t_statistic_threshold
                """:type next_t_statistic_selector: np.ndarray """
            else:
                break

            # stop iterating if there are no changes in set of selected experiments
            if np.all(next_t_statistic_selector[t_statistic_selector]):
                break  # t_statistic_selector shouldn't be updated

            # update experiment selector
            t_statistic_selector = np.logical_and(t_statistic_selector, next_t_statistic_selector)

            if not np.any(t_statistic_selector):
                raise Exception("Value of parametr t_statistic_threshold is too small")

        number_of_selected_by_t_statistic = np.sum(t_statistic_selector)

        chi_numerator_arr = t_statistic[t_statistic_selector] ** 2
        """:type chi_numerator_arr: np.ndarray """
        chi_numerator = np.sum(chi_numerator_arr)

        chi_degs_of_freedom = number_of_selected_by_t_statistic - 1 - self.n_degrees_of_freedom

        params.set_additional_info("{},{}".format(float(model.intercept_), float(model.coef_)))

        chi_squared = chi_numerator / float(chi_degs_of_freedom)

        logging.debug("ChiSquared = %s", chi_squared)
        logging.info("Num of selected by t-statistic = %s", number_of_selected_by_t_statistic)
        logging.info("Num of discarded by t-statistic = %s", len(t_statistic_selector) - number_of_selected_by_t_statistic)
        return chi_squared
