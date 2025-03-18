import datetime
import pytest

import yaqutils.time_helpers as utime

from adminka import ab_observation


# noinspection PyClassHasNoInit
class TestParseObservation:
    def test_basic(self, session):
        obs_data = {
            "datestart": "20160101",
            "dateend": "20160102",
            "obs_id": 12345,
            "testids": [6, 7, 8],
            "uid": "uid",
            "filters": []
        }
        obs = ab_observation.parse_observation(obs_data, session)
        assert obs.dates == utime.DateRange(
            datetime.date(2016, 1, 1),
            datetime.date(2016, 1, 2)
        )

        assert obs.id == "12345"
        assert obs.control.testid == "6"
        assert obs.all_testids(include_control=False) == ["7", "8"]

    def test_no_id(self, session):
        obs_data = {
            "datestart": "20160101",
            "dateend": "20160102",
            "testids": [6, 7, 8],
            "uid": "uid",
            "filters": []
        }
        obs = ab_observation.parse_observation(obs_data, session)
        assert obs.id is None

    def test_no_testids(self, session):
        obs_data = {
            "datestart": "20160101",
            "dateend": "20160102",
            "obs_id": 12345,
            "testids": [],
        }
        with pytest.raises(ab_observation.ObservationParseException):
            ab_observation.parse_observation(obs_data, session)

        obs_data = {
            "datestart": "20160101",
            "dateend": "20160102",
            "obs_id": 12345,
            "uid": "uid",
            "testids": [1],
            "filters": []
        }
        ab_observation.parse_observation(obs_data, session)

    def test_no_dates(self, session):
        obs_data = {
            "obs_id": 12345,
            "testids": [6, 7, 8],
            "uid": "uid",
            "filters": []
        }
        obs = ab_observation.parse_observation(obs_data, session)
        assert obs.dates.start is None and obs.dates.end is None

    def test_incorrect_dates(self, session):
        obs_data = {
            "datestart": "20160102",
            "dateend": "20160101",
            "obs_id": 12345,
            "testids": [6, 7, 8],
            "uid": "uid",
            "filters": []
        }
        with pytest.raises(ab_observation.ObservationParseException):
            ab_observation.parse_observation(obs_data, session)

    def test_one_day(self, session):
        obs_data = {
            "datestart": "20160101",
            "dateend": "20160101",
            "obs_id": 12345,
            "testids": [6, 7, 8],
            "uid": "uid",
            "filters": []
        }
        assert ab_observation.parse_observation(obs_data, session)
