import pytest


@pytest.fixture
def data_path(request):
    try:
        import yatest.common
        return str(yatest.common.source_path("tools/mstand/session_metric/tests/ut/data"))
    except:
        return str(request.config.rootdir.join("session_metric/tests/ut/data"))
