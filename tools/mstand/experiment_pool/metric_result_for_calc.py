import yaqutils.misc_helpers as umisc

from experiment_pool import ExperimentForCalculation
from experiment_pool import MetricResult  # noqa


# this class identifies a unit for criteria calculation
@umisc.hash_and_ordering_from_key_method
class MetricResultForCalculation(object):
    def __init__(self, exp_for_calc, metric_result):
        """
        :type exp_for_calc: ExperimentForCalculation
        :type metric_result: MetricResult
        """
        self.exp_for_calc = exp_for_calc
        self.metric_result = metric_result

    def __str__(self):
        return "MRForCalc{}".format(self.key())

    def __repr__(self):
        return str(self)

    def key(self):
        return self.exp_for_calc.key(), self.metric_result.key()

    @staticmethod
    def from_observation(observation):
        """
        :type: observation: Observation
        :rtype: set[MetricResultForCalculation]
        """
        exps_for_calc = ExperimentForCalculation.from_observation(observation)
        metric_results_for_calc = set()
        for one_exp_for_calc in exps_for_calc:
            for metric_result in one_exp_for_calc.experiment.metric_results:
                one_mr_for_calc = MetricResultForCalculation(one_exp_for_calc, metric_result)
                metric_results_for_calc.add(one_mr_for_calc)
        return metric_results_for_calc

    @staticmethod
    def from_pool(pool):
        """
        :type: pool: Pool
        :rtype: set[MetricResultForCalculation]
        """
        metric_results_for_calc = set()
        for observation in pool.observations:
            metric_results_for_calc.update(MetricResultForCalculation.from_observation(observation))
        return metric_results_for_calc
