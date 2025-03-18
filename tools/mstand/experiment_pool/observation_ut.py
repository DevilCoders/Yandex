import datetime
import logging

import yaqutils.time_helpers as utime
from experiment_pool import Experiment
from experiment_pool import Observation

TEST_DATES = utime.DateRange(
    datetime.date(2016, 1, 1),
    datetime.date(2016, 4, 1),
)


def generate_observation(obs_id='1',
                         dates=TEST_DATES,
                         control_id='1',
                         experiment_ids=None,
                         tags=None):
    """
    Generates observation for tests
    :type obs_id: str | None
    :type dates: utime.DateRange
    :type control_id: str
    :type experiment_ids: iter[str]
    :type tags: list[str] | None
    :return:
    """
    if experiment_ids is None:
        experiment_ids = []
    if tags is None:
        tags = []
    control = Experiment(testid=control_id)
    experiments = [Experiment(testid=exp) for exp in experiment_ids]
    return Observation(
        obs_id=obs_id,
        dates=dates,
        control=control,
        experiments=experiments,
        tags=tags
    )


# noinspection PyClassHasNoInit
class TestObservationValidation:
    def test_endless_observation_valid(self):
        dates = utime.DateRange(datetime.date(2016, 1, 1), None)
        generate_observation(obs_id="1001", dates=dates, control_id="1", experiment_ids=["1234", "1235", "1236"])


# noinspection PyClassHasNoInit
class TestObservationOperators:
    def test_equals(self):
        testids = ["1111", "2222", "3333"]
        obs1 = generate_observation(obs_id="1001", control_id="1", experiment_ids=testids)
        obs2 = generate_observation(obs_id="1002", control_id="2", experiment_ids=testids)
        obs3 = generate_observation(obs_id="1001", control_id="1", experiment_ids=testids)
        logging.info("\n%s\n%s\n%s\n", obs1.key(), obs2.key(), obs3.key())
        # logging.info(obs1.key())
        # logging.info(obs2.key())
        # logging.info(obs3.key())
        assert (obs1.key() != obs2.key())
        assert (obs1.key() == obs3.key())
        assert (obs1 != obs2)
        assert (obs1 == obs3)

    def test_repr(self):
        testids = ["1111", "2222", "3333"]
        obs = generate_observation(obs_id="1000", control_id="5678", experiment_ids=testids)

        obs_repr = "{}".format(obs)
        assert ("1000" in obs_repr)
        assert ("20160101" in obs_repr)
        assert ("20160401" in obs_repr)

    def test_get_testids(self):
        testids = ["1111", "2222", "3333"]
        obs = generate_observation(obs_id="1000", control_id="5678", experiment_ids=testids)
        assert (obs.all_testids() == testids + ["5678"])

    def test_all_data_types(self):
        testids = ["1111", "2222", "3333"]
        obs = generate_observation(obs_id="1000", control_id="5678", experiment_ids=testids)
        assert obs.all_data_types() == set()

    def test_observation_clone(self):
        testids = ["1111", "2222", "3333"]
        obs = generate_observation(obs_id="1000", control_id="5678", experiment_ids=testids)
        obs_clone = obs.clone(clone_experiments=False, clone_metric_results=False)
        assert obs_clone is not obs
        assert not obs_clone.all_experiments()


# noinspection PyClassHasNoInit
class TestObservationSerialize:
    def test_serialize(self):
        testids = ["1111", "2222", "3333"]
        obs = generate_observation(obs_id="1001", control_id="1", experiment_ids=testids)
        serialized = {
            "observation_id": "1001",
            "date_from": "20160101",
            "date_to": "20160401",
            "control": {
                "testid": "1",
            },
            "experiments": [
                {
                    "testid": "1111",
                },
                {
                    "testid": "2222",
                },
                {
                    "testid": "3333",
                },
            ]
        }
        assert serialized == obs.serialize()

    def test_serialize_no_dates(self):
        control = Experiment(testid="1234")
        obs = Observation(dates=None, obs_id="100500", control=control)
        obs.serialize()
