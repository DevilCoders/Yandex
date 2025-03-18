# coding=utf-8

from copy import deepcopy
from datetime import datetime, timedelta

import pytest
from hamcrest import assert_that, equal_to

from antiadblock.cryprox.cryprox.common.tools.experiments import get_active_experiment
from antiadblock.cryprox.cryprox.config.system import EXPERIMENT_START_TIME_FMT, Experiments, UserDevice


@pytest.mark.parametrize("shift_day", (0, 1))
def test_get_active_experiment_from_one_experiment_without_weekday(shift_day):
    now = datetime.utcnow().replace(microsecond=0)
    exp_1 = {
        "EXPERIMENT_TYPE": Experiments.BYPASS,
        "EXPERIMENT_START": (now - timedelta(days=shift_day)).strftime(EXPERIMENT_START_TIME_FMT),
        "EXPERIMENT_PERCENT": 10,
        "EXPERIMENT_DURATION": 5,
        "EXPERIMENT_DEVICE": [UserDevice.DESKTOP],
    }
    active_experiment = get_active_experiment([exp_1])

    if not shift_day:
        # experiment is active
        exp_1["EXPERIMENT_START"] = now
        assert_that(active_experiment, equal_to(exp_1))
    else:
        # experiment is not active
        assert active_experiment == {}


@pytest.mark.parametrize("shift_weekday", (0, 1))
def test_get_active_experiment(shift_weekday):
    now = datetime.utcnow().replace(microsecond=0)
    weekday = now.weekday()
    exp_1 = {
        "EXPERIMENT_TYPE": 1,
        "EXPERIMENT_START": (now - timedelta(days=7)).strftime(EXPERIMENT_START_TIME_FMT),
        "EXPERIMENT_PERCENT": 10,
        "EXPERIMENT_DURATION": 5,
        "EXPERIMENT_DEVICE": [0],
        "EXPERIMENT_DAYS": [(weekday + shift_weekday) % 7],
    }
    exp_2 = deepcopy(exp_1)
    exp_2["EXPERIMENT_DAYS"] = [(weekday + shift_weekday + 1) % 7]
    active_experiment = get_active_experiment([exp_1, exp_2])
    if not shift_weekday:
        # experiment is active
        exp_1["EXPERIMENT_START"] = now
        assert_that(active_experiment, equal_to(exp_1))
    else:
        # experiment is not active
        assert active_experiment == {}
