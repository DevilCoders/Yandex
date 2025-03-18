import pytest


@pytest.fixture
def data_path(request):
    try:
        import yatest.common
        return str(yatest.common.source_path("tools/mstand/sample_metrics/online/tests/data"))
    except:
        return str(request.config.rootdir.join("sample_metrics/online/tests/data"))
