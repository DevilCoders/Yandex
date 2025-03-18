import pytest


@pytest.fixture
def data_path(request):
    try:
        import yatest.common
        return str(yatest.common.source_path("tools/mstand/postprocessing/tests/ut/data"))
    except:
        return str(request.config.rootdir.join("postprocessing/tests/ut/data"))
