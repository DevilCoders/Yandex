import pkgutil
from json import loads
from os.path import join, dirname

import pytest
from dssclient import Dss
from dssclient.http import HttpConnector

try:
    import library.python
    ARCADIA_RUN = True

except ImportError:
    ARCADIA_RUN = False


class FakeDss(Dss):

    def set_response(self, val):
        self.connector.response = val


class FakeConnector(HttpConnector):

    response = None

    def __init__(self, auth=None, response=None):
        if self.response is None:
            self.response = response or {}

    def request(self, *args, **kwargs):
        return self.response


@pytest.fixture
def fake_connector():

    def fake(response=None):
        return FakeConnector(response=response)

    return fake


@pytest.fixture
def fake_dss():

    def fake(response=None):
        return FakeDss(FakeConnector(response=response))

    return fake


@pytest.fixture
def read_fixture(request):

    fixtures_path = join(dirname(request.module.__file__), 'fixtures')

    def read_fixture_(filename, load_json=True):
        file_path = join(fixtures_path, filename)

        if ARCADIA_RUN:
            data = pkgutil.get_data(__package__, file_path)

        else:
            with open(file_path, 'rb') as f:
                data = f.read()

        if load_json:
            data = loads(data)

        return data

    return read_fixture_
