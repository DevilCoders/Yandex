import os
import pathlib
from unittest.mock import patch

import pytest
from rich.prompt import Confirm

import common.tests.conftest
import api.tests.conftest


@pytest.fixture(scope="function")
def testdata(request):
    cwd = pathlib.Path.cwd()
    os.chdir(pathlib.Path(__file__).parent / "testdata")
    yield
    os.chdir(cwd.as_posix())


@pytest.fixture(scope="function")
def mocked_confirmation():
    with patch.object(Confirm, "ask", return_value=True):
        yield


mocked_s3 = common.tests.conftest.mocked_s3
test_database = common.tests.conftest.test_database
client = api.tests.conftest.client
