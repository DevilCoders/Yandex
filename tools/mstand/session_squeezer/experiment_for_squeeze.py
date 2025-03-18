from typing import List
from typing import Optional
from typing import Tuple

import experiment_pool
import mstand_utils.testid_helpers as utestid
import session_squeezer.services as squeezer_services
import yaqutils.misc_helpers as umisc
import yaqutils.time_helpers as utime

from experiment_pool import Experiment
from experiment_pool import Observation
from experiment_pool import ObservationFilters


@umisc.hash_and_ordering_from_key_method
class ExperimentForSqueeze:
    def __init__(self,
                 experiment: Experiment,
                 observation: Observation,
                 service: str,
                 all_users: Optional[bool] = None,
                 all_for_history: Optional[bool] = False):
        self.experiment = experiment.clone()
        self.observation = observation.clone()
        self.service = service
        self.use_any_filter = squeezer_services.SQUEEZERS[self.service].USE_ANY_FILTER

        if all_users is None:
            all_users = utestid.testid_is_all(self.experiment.testid)
        self.all_users = all_users

        self.all_for_history = all_for_history

        if self.all_users and self.all_for_history:
            raise Exception("Can't use history/future with all-users testid: {}".format(self.experiment))

    def __str__(self) -> str:
        return "<{}, {}>".format(self.experiment.testid, self.service)

    def __repr__(self) -> str:
        return "ExperimentForSqueeze({!r}, {!r}, {!r})".format(self.experiment, self.observation, self.service)

    @property
    def filter_hash(self) -> Optional[str]:
        if self.filters:
            return self.filters.filter_hash
        return None

    @property
    def testid(self) -> Optional[str]:
        return self.experiment.testid

    @property
    def filters(self) -> ObservationFilters:
        return self.observation.filters if squeezer_services.has_filter_support(self.service) else ObservationFilters()

    @property
    def dates(self) -> Optional[utime.DateRange]:
        return self.observation.dates

    @property
    def has_triggered_testids_filter(self) -> bool:
        return self.observation.filters.has_triggered_testids_filter

    def key(self) -> Tuple[str, Optional[str], Tuple[Tuple[Tuple[str, str], ...], Optional[str]]]:
        return self.service, self.testid, self.filters.key()

    @staticmethod
    def from_observation(observation: Observation) -> List["ExperimentForSqueeze"]:
        assert observation.services

        result = []
        for experiment in observation.all_experiments():
            for service in observation.services:
                result.append(ExperimentForSqueeze(experiment, observation, service))
        return result

    @staticmethod
    def from_pool(pool: experiment_pool.Pool) -> List["ExperimentForSqueeze"]:
        all_exps = []
        for observation in pool.observations:
            all_exps.extend(ExperimentForSqueeze.from_observation(observation))
        return all_exps
