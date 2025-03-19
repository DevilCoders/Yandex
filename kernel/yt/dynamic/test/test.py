import subprocess
import pytest

import yatest.common
from mapreduce.yt.python.yt_stuff import YtConfig
from mapreduce.yt.python.yt_stuff import yt_stuff   # noqa

BINARY_PATH = yatest.common.binary_path("kernel/yt/dynamic/ut/ut")
TESTS_LIST = sorted(subprocess.check_output([BINARY_PATH, "--list-verbose"]).split())
TEST_ID = "kernel_yt_dynamic"


@pytest.fixture(scope="module")
def yt_config(request):
    return YtConfig(
        yt_id=TEST_ID,
        wait_tablet_cell_initialization=True,
        job_controller_resource_limits={"memory": 4 * 1024 * 1024 * 1024},
    )


@pytest.mark.parametrize("test_id", TESTS_LIST, ids=lambda x: x.replace("::Test", ""))  # noqa
def test(yt_stuff, yt_config, test_id):
    assert yt_stuff.config.yt_id == TEST_ID
    res = yatest.common.execute(
        [BINARY_PATH, '+%s' % test_id],
        collect_cores=True,
        env={"YT_PREFIX": "//tmp/%s/%s" % (TEST_ID, test_id),
             "YT_USER": "root",
             "YT_PROXY": yt_stuff.get_server()},
    )

    if res.std_out:
        return res.std_out
