import pytest
import yatest
import vcr

from async_clients.clients.base import BaseClient


@pytest.fixture
def test_vcr():
    path = yatest.common.source_path('library/python/async_clients/vcr_cassettes')
    return vcr.VCR(
        cassette_library_dir=path,
    )


@pytest.fixture
def dummy_client():
    class DummyClient(BaseClient):
        pass
    return DummyClient
