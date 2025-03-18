import pytest
import random


def pytest_addoption(parser):
    # used for criterias_ut_fat
    parser.addoption("--seed",
                     type=int,
                     default=random.randrange(0, 2 ** 32),
                     help="specify random seed")


@pytest.fixture
def project_path(request) -> str:
    try:
        # noinspection PyUnresolvedReferences
        import yatest.common.runtime
        rootdir = yatest.common.runtime._get_ya_config().rootdir
    except ImportError:
        rootdir = request.config.rootdir

    return str(rootdir)
