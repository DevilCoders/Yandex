# -*- coding: utf-8 -*-

import datetime

import yaqutils.time_helpers as utime
import yaqutils.six_helpers as usix
from experiment_pool import Experiment
from experiment_pool import Observation
from metrics_api import ExperimentForAPI
from metrics_api import ObservationForAPI
from metrics_api.online import UserActionForAPI
from metrics_api.online import UserActionsForAPI


# noinspection PyClassHasNoInit
class TestUserActions:
    dates = utime.DateRange(datetime.date(2016, 5, 2), datetime.date(2016, 5, 2))

    control = Experiment(testid="all")

    experiment = Experiment(testid="all")
    experiment_api = ExperimentForAPI(experiment)

    observation = Observation(obs_id=None, dates=dates,
                              control=control, experiments=[experiment])
    observation_api = ObservationForAPI(observation)

    data = {"Test": u"привет", "without": "arrays"}
    other_data = {"Tiny": "dictionary"}
    corrupted_data = {"query": "text\xff\xed\xe4\xe5\xea\xf1\xfe\xea\xf3from"}

    action = UserActionForAPI("y1337", 1465133772, data)
    other_action = UserActionForAPI("y1337", 1590009772, other_data)
    corrupted_action = UserActionForAPI("y1337", 1462153777, corrupted_data)

    def test_single_action_dump(self):
        log_dump = self.action.dump()

        assert u"UserActionForAPI" in log_dump

        for key, value in usix.iteritems(self.data):
            assert key in log_dump
            assert value in log_dump

    def test_multiple_actions_dump(self):
        user_api = UserActionsForAPI(
            user="y1337",
            actions=[self.action, self.other_action],
            experiment=self.experiment_api,
            observation=self.observation_api,
        )
        log_dump = user_api.dump()

        assert u"UserActionsForAPI" in log_dump

        for key, value in usix.iteritems(self.other_data):
            assert key in log_dump
            assert value in log_dump

    def test_corrupted_data_dump(self):
        log_dump = self.corrupted_action.dump()

        assert u"UserActionForAPI" in log_dump, "UserActionForAPI text not found"

    def test_clone(self):
        api1 = UserActionsForAPI(
            user="y1337",
            actions=[self.action, self.other_action],
            experiment=self.experiment_api,
            observation=self.observation_api,
        )
        api2 = api1.clone()
        assert id(api1) != id(api2)
        assert api1.user == api2.user
        assert len(api1.actions) == len(api2.actions)
        assert all(id(a1) != id(a2) for a1, a2 in zip(api1.actions, api2.actions))
        assert all(id(a1.data) != id(a2.data) for a1, a2 in zip(api1.actions, api2.actions))
        assert all(a1.data == a2.data for a1, a2 in zip(api1.actions, api2.actions))
        assert all(a1.timestamp == a2.timestamp for a1, a2 in zip(api1.actions, api2.actions))
