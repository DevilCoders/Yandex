from typing import TYPE_CHECKING

import pytest

from cloud.dwh.test_utils.yt.misc import get_test_prefix

if TYPE_CHECKING:
    from yt.wrapper import YtClient
    from yql.api.v1.client import YqlClient


@pytest.fixture
def yql_client(tmpdir, yql_api, mongo) -> 'YqlClient':
    from yql.api.v1.client import YqlClient

    with YqlClient(server='localhost', port=yql_api.port, db='plato') as client:
        yield client


@pytest.fixture
def yt_client(yt) -> 'YtClient':
    client = yt.get_yt_client()  # type: 'YtClient'
    yield client


@pytest.fixture
def test_prefix() -> 'str':
    yield get_test_prefix()
