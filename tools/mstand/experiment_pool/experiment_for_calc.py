import yaqutils.misc_helpers as umisc

from experiment_pool import Experiment
from experiment_pool import Observation


@umisc.hash_and_ordering_from_key_method
class ExperimentForCalculation(object):
    def __init__(self, experiment, observation, experiment_results=False):
        """
        :type experiment: Experiment
        :type observation: Observation
        :type experiment_results: bool
        """
        assert isinstance(experiment, Experiment)
        assert isinstance(observation, Observation)
        self.experiment = experiment.clone(clone_metric_results=experiment_results)
        self.observation = observation.clone()

    @property
    def testid(self):
        return self.experiment.testid

    @property
    def dates(self):
        return self.observation.dates

    @property
    def filters(self):
        return self.observation.filters

    def __str__(self):
        if self.observation.id is not None:
            return "ExpForCalc(t={}, s={}, dates={}, obs={})".format(self.experiment.testid,
                                                                     self.experiment.serpset_id,
                                                                     self.dates,
                                                                     self.observation.id)
        else:
            return "ExpForCalc(t={}, s={}, dates={})".format(self.experiment.testid,
                                                             self.experiment.serpset_id,
                                                             self.dates)

    def __repr__(self):
        return str(self)

    def key(self):
        return self.experiment.key(), self.dates.key(), self.filters.key(), repr(self.observation.extra_data or "")

    @staticmethod
    def from_observation(observation, experiment_results=False):
        """
        :type observation: Observation
        :type experiment_results: bool
        :rtype: set[ExperimentForCalculation]
        """
        if observation.dates.is_infinite():
            return set()
        return {ExperimentForCalculation(experiment, observation, experiment_results=experiment_results)
                for experiment in observation.all_experiments()
                if experiment.testid is not None}

    @staticmethod
    def from_pool(pool, experiment_results=False):
        """
        :type pool: Pool
        :type experiment_results: bool
        :rtype: set[ExperimentForCalculation]
        """
        exps_for_calc = set()
        for observation in pool.observations:
            exps_for_calc.update(ExperimentForCalculation.from_observation(observation,
                                                                           experiment_results=experiment_results))
        return exps_for_calc
