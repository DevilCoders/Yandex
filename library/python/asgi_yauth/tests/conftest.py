import pytest
import yatest
import vcr

from fastapi.testclient import TestClient
from fastapi import FastAPI


@pytest.fixture
def test_vcr():
    path = yatest.common.source_path('library/python/asgi_yauth/vcr_cassettes')
    return vcr.VCR(
        cassette_library_dir=path,
    )


@pytest.fixture
def app():
    app = FastAPI(
        title='test_app',
    )
    return app


@pytest.fixture
def client(app):
    with TestClient(app) as client:
        yield client
