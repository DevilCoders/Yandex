import adminka.adminka_helpers as admhelp
from experiment_pool import Experiment, Observation


# noinspection PyClassHasNoInit
class TestSafeAdminkaFetchMethods:
    def test_fetch_observation_ticket_good(self, session):
        obs = Observation(obs_id="4", control=None, dates=None)

        obs_ticket = admhelp.fetch_observation_ticket(session=session, obs=obs)
        assert "ERROR" not in obs_ticket

    def test_fetch_observation_ticket_bad(self, session):
        obs = Observation(obs_id="3", control=None, dates=None)
        # no ticket in this observation
        obs_ticket = admhelp.fetch_observation_ticket(session=session, obs=obs)
        assert "ABT_TEMP_ERROR" in obs_ticket

    def test_fetch_abt_experiment_field_good(self, session):
        exp = Experiment(testid="1")
        # no ticket in this observation
        exp_ticket = admhelp.fetch_abt_experiment_field(session=session, exp=exp, field_name="ticket")
        assert exp_ticket == "QQQ-123"

    def test_fetch_abt_experiment_field_bad(self, session):
        exp = Experiment(testid="1")
        # no ticket in this observation
        exp_ticket = admhelp.fetch_abt_experiment_field(session=session, exp=exp, field_name="non-existng-field")
        assert exp_ticket is None
