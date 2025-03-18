import os.path
import pytest

from adminka.ab_cache import AdminkaCachedApi


@pytest.fixture
def data_path(request):
    try:
        import yatest.common
        return str(yatest.common.source_path("tools/mstand/adminka/tests/data"))
    except:
        return str(request.config.rootdir.join("adminka/tests/data"))


@pytest.fixture
def session(data_path, monkeypatch):
    def preload_testids(this, testids):
        setattr(this, "testids_preloaded", True)

    monkeypatch.setattr(AdminkaCachedApi, "preload_testids", preload_testids)

    return AdminkaCachedApi(path=os.path.join(data_path, "cache.json"))
