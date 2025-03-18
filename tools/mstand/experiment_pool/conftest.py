import pytest


@pytest.fixture
def data_path(request):
    try:
        import yatest.common
        return str(yatest.common.source_path("tools/mstand/experiment_pool/tests/data"))
    except:
        return str(request.config.rootdir.join("experiment_pool/tests/data"))
