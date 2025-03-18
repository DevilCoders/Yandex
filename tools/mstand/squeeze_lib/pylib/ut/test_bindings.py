import pytest

from experiment_pool import Experiment
from experiment_pool import Observation
from experiment_pool import ObservationFilters

from mstand_utils.yt_options_struct import YtJobOptions
from session_squeezer.experiment_for_squeeze import ExperimentForSqueeze
from session_yt.squeeze_yt import ServiceParams

from tools.mstand.squeeze_lib.pylib import create_temp_table, get_squeeze_versions, squeeze_day


@pytest.fixture
def experiments_for_squeeze():
    exp = Experiment(testid="1")
    filters = [("name_1", "value_1"), ("name_2", "value_2")]
    obs_filters = ObservationFilters(filter_hash="filter_hash", filters=filters)
    obs = Observation(obs_id=None, dates=None, control=exp, filters=obs_filters)
    exp_for_squeeze = ExperimentForSqueeze(exp, obs, "web")

    return [
        exp_for_squeeze,
        exp_for_squeeze,
    ]


@pytest.fixture
def service_params():
    yt_job_options = YtJobOptions(
        pool="test-pool",
    )

    return ServiceParams(
        destinations=None,
        squeezer=None,
        paths=None,
        local_files=None,
        versions=None,
        cache_versions=None,
        client=None,
        yt_job_options=yt_job_options,
        transform=None,
        table_bounds=None,
        allow_empty_tables=None,
        sort_threads=None,
        day=None,
        add_acl=None,
        operation_sid=None,
        squeeze_backend=None,
    )


def test_create_temp_table():
    assert create_temp_table(b"service") == b"//tmp/table/service"


def test_get_squeeze_versions():
    assert get_squeeze_versions("web") == {"_common": 1, "web": 1}


def test_get_unknown_service_version():
    with pytest.raises(Exception):
        get_squeeze_versions("unknown-service")


@pytest.skip("TODO: skip reason")
def test_squeeze_day(experiments_for_squeeze, service_params):
    assert squeeze_day(experiments_for_squeeze, service_params) == "pool: test-pool, experiments_for_squeeze: 2"
