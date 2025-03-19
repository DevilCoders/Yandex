import logging.config

import pytest
from fastapi.testclient import TestClient
from fastapi_cache import FastAPICache
from fastapi_cache.backends.inmemory import InMemoryBackend

import api
import api.dependencies
import common.tests.conftest
import settings
from api.client import ModelBasedHttpClient

test_database = common.tests.conftest.test_database
mocked_s3 = common.tests.conftest.mocked_s3
db = common.tests.conftest.db

api.app.dependency_overrides = {
    api.dependencies.db: common.tests.conftest.override_db
}

# Disable FastAPI's caches for tests
FastAPICache.init(InMemoryBackend(), enable=False)


@pytest.fixture
def client(test_database) -> ModelBasedHttpClient:
    """
    Returns authenticated TestClient with response model validation
    to API application with initialized database.

    See docs for `api.client.ModelBasedHttpClient` for details.
    """
    client = ModelBasedHttpClient(TestClient(api.app))
    client.headers = {"X-API-KEY": settings.API_KEY}
    return client


@pytest.fixture
def non_authenticated_client(test_database) -> ModelBasedHttpClient:
    """
    Returns non-authenticated TestClient to API application with initialized database.
    """
    return ModelBasedHttpClient(TestClient(api.app))


# Setup logging for tests in the same way as for applications
logging.config.dictConfig(settings.LOGGING_CONFIG)
