from functools import partial

import pytest

from bclclient import Bcl, HOST_TEST
from bclclient.settings import TVM_ID_TEST

try:
    from envbox import get_environment

    environ = get_environment()

except ImportError:
    from os import environ


TVM_SECRET = environ.get('TVM_SECRET', '')


@pytest.fixture(scope='session')
def bypass_mock():
    return bool(TVM_SECRET)


@pytest.fixture(scope='session')
def acc_bank() -> str:
    return '40702810538000111471'


@pytest.fixture(scope='session')
def acc_psys() -> str:
    return '200765'


class DummyAuth:

    def get_ticket(self):
        return 'dummy'

    @classmethod
    def from_arg(cls, *args, **kwarg):
        return DummyAuth()


@pytest.fixture
def bcl_test(bypass_mock, monkeypatch) -> Bcl:
    if not bypass_mock:
        monkeypatch.setattr('bclclient.auth.TvmAuth.from_arg', DummyAuth.from_arg)
    return Bcl(auth=(TVM_ID_TEST, TVM_SECRET), host=HOST_TEST)


@pytest.fixture
def set_service_alias():

    def set_service_alias_(*, client: Bcl, alias: str):
        client._connector._service_alias = alias

    return set_service_alias_


@pytest.fixture
def response_mocked(response_mock, bypass_mock):
    return partial(response_mock, bypass=bypass_mock)
