import pytest
import requests_mock
from cloud.mdb.salt.salt._modules import compute_metadata


def test_good():
    compute_metadata.__context__ = {}
    with requests_mock.Mocker() as m:
        m.get(
            'http://169.254.169.254/computeMetadata/v1/instance/service-accounts/default/token',
            text='{"access_token": "TEST_IAM_TOKEN", "expires_in": 100500}',
        )
        assert compute_metadata.iam_token() == "TEST_IAM_TOKEN"


def test_no_service_account_on_vm():
    compute_metadata.__context__ = {}
    with requests_mock.Mocker() as m:
        m.get(
            'http://169.254.169.254/computeMetadata/v1/instance/service-accounts/default/token',
            status_code=404,
            text='',
        )
        with pytest.raises(compute_metadata.CommandExecutionError):
            compute_metadata.iam_token()


def test_metadata_unavailable():
    compute_metadata.__context__ = {}
    with requests_mock.Mocker() as m:
        m.get(
            'http://169.254.169.254/computeMetadata/v1/instance/service-accounts/default/token',
            status_code=502,
            text='',
        )
        with pytest.raises(compute_metadata.CommandExecutionError):
            compute_metadata.iam_token()
