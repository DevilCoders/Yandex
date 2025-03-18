import os
import pytest

from adminka.ab_cache import AdminkaCachedApi


@pytest.fixture
def root_path(request):
    try:
        import yatest.common
        return str(yatest.common.source_path("tools/mstand"))
    except:
        return str(request.config.rootdir)


@pytest.fixture
def data_path(root_path):
    return os.path.join(root_path, "reports/tests/data")


@pytest.fixture(scope="function")
def session(root_path):
    cache_path = os.path.join(root_path, "adminka/tests/data/cache.json")
    return AdminkaCachedApi(path=str(cache_path))
