import datetime

import yaqutils.time_helpers as utime
from experiment_pool import Experiment
from experiment_pool import ExperimentForCalculation
from experiment_pool import Observation
from experiment_pool import ObservationFilters
from experiment_pool import Pool

DATE_FROM = datetime.date(2016, 1, 1)
DATE_TO = datetime.date(2016, 1, 2)
TEST_DATES = utime.DateRange(DATE_FROM, DATE_TO)

DATE_TO_ALT = datetime.date(2016, 1, 3)
TEST_DATES_ALT = utime.DateRange(DATE_FROM, DATE_TO_ALT)


# noinspection PyClassHasNoInit
class TestExperimentForCalculationOperators:
    def test_exp_for_calc_equal(self):
        exp1 = Experiment(testid="123")
        exp2 = Experiment(testid="456")
        control = Experiment(testid="123")
        obs = Observation(obs_id="1234", dates=TEST_DATES, control=control)
        exp_for_calc1 = ExperimentForCalculation(experiment=exp1, observation=obs)
        exp_for_calc2 = ExperimentForCalculation(experiment=exp2, observation=obs)

        assert (exp_for_calc1 != exp_for_calc2)

    def test_exp_for_calc_hash(self):
        exp1 = Experiment(testid="123")
        exp2 = Experiment(testid="456")
        control = Experiment(testid="123")
        obs = Observation(obs_id="1234", dates=TEST_DATES, control=control)
        exp_for_calc1 = ExperimentForCalculation(experiment=exp1, observation=obs)
        exp_for_calc2 = ExperimentForCalculation(experiment=exp2, observation=obs)

        exp_map = {exp_for_calc1: 1,
                   exp_for_calc2: 2}

        assert len(exp_map) == 2

    def test_exp_for_calc_operators(self):
        control = Experiment(testid="123")
        obs = Observation(obs_id="1234", dates=TEST_DATES, control=control)
        exp_for_calc = ExperimentForCalculation(experiment=control, observation=obs)
        assert "2016" in "{}".format(exp_for_calc)
        assert "1234" in "{}".format(exp_for_calc)
        assert "ExpForCalc(t=123" in "{}".format(exp_for_calc)

    def test_from_observation_same_testids(self):
        exp1 = Experiment(testid="123")
        exp2 = Experiment(testid="456")
        control = Experiment(testid="123")
        obs = Observation(obs_id="1234", dates=TEST_DATES, control=control, experiments=[exp1, exp2])

        exps_for_calc = ExperimentForCalculation.from_observation(obs)
        # control is equal to exp1
        assert len(exps_for_calc) == 2

    def test_from_observation_different_testids(self):
        exp1 = Experiment(testid="123")
        exp2 = Experiment(testid="456")
        control = Experiment(testid="321")
        obs = Observation(obs_id="1234", dates=TEST_DATES, control=control, experiments=[exp1, exp2])

        exps_for_calc = ExperimentForCalculation.from_observation(obs)
        assert len(exps_for_calc) == 3

    def test_from_pool(self):
        exp11 = Experiment(testid="111")
        exp12 = Experiment(testid="222")
        control1 = Experiment(testid="111")
        obs1 = Observation(obs_id="1234", dates=TEST_DATES, control=control1, experiments=[exp11, exp12])
        control2 = Experiment(testid="222")
        exp21 = Experiment(testid="333")
        exp22 = Experiment(testid="333")
        obs2 = Observation(obs_id="5678", dates=TEST_DATES, control=control2, experiments=[exp21, exp22])

        pool = Pool([obs1, obs2])

        exps_for_calc = ExperimentForCalculation.from_pool(pool)
        # control1 = exp11
        # exp21 = exp22
        # total - 6, unique - 3
        assert len(exps_for_calc) == 3

    def test_from_pool_with_filter(self):
        exp11 = Experiment(testid="111")
        exp12 = Experiment(testid="222")
        control1 = Experiment(testid="111")
        filters1 = ObservationFilters([("filter1", "value1")], "hash1")
        obs1 = Observation(obs_id="1234", dates=TEST_DATES, control=control1, experiments=[exp11, exp12],
                           filters=filters1)
        control2 = Experiment(testid="222")
        exp21 = Experiment(testid="333")
        exp22 = Experiment(testid="333")
        filters2 = ObservationFilters([("filter2", "value2")], "hash2")
        obs2 = Observation(obs_id="5678", dates=TEST_DATES, control=control2, experiments=[exp21, exp22],
                           filters=filters2)

        pool = Pool([obs1, obs2])

        exps_for_calc = ExperimentForCalculation.from_pool(pool)
        # control1 = exp11
        # exp21 = exp22
        # total - 6, unique - 4
        assert len(exps_for_calc) == 4
