from dssclient import Dss
from dssclient.auth import BasicAuth, OwnerAuth, StaticAuth
from dssclient.exceptions import DssClientException

import pytest


def test_basic():

    auth = BasicAuth(username='U', password='P')
    header = auth.get_auth_header()

    assert 'Basic' in header
    assert auth.get_auth_header(renew=True) == header


def test_static():

    auth = StaticAuth(access_token='mytoken')
    header = auth.get_auth_header()

    assert 'Bearer' in header
    assert 'mytoken' in header

    with pytest.raises(DssClientException):
        auth.get_auth_header(renew=True)


def test_owner(fake_connector):

    auth = OwnerAuth(client_id='clientid', username='U', password='P')
    assert not auth.get_auth_header()

    auth = OwnerAuth(client_id='clientid', username='U', password='P', access_token='sometoken')
    header = auth.get_auth_header()

    assert 'Bearer' in header
    assert 'sometoken' in header

    auth.connector = fake_connector()
    auth.connector.response = {'access_token': 'newtoken'}
    header = auth.get_auth_header(renew=True)
    auth.connector.response = None

    assert 'Bearer' in header
    assert 'newtoken' in header


def test_request(response_mock):

    dss = Dss('abcdf')

    with response_mock('GET https://dss.yandex-team.ru/SignServer/rest/api/certificates -> 200:{}'):
        certs = dss.certificates.get_all()
        assert not certs

    with response_mock('GET https://dss.yandex-team.ru/SignServer/rest/api/certificates -> 401:{}'):
        with pytest.raises(DssClientException) as e:
            dss.certificates.get_all()
        assert f'{e.value}' == 'Your static token is invalid and StaticAuth is unable to renew access token.'
