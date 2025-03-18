import pytest
from mapreduce.yt.python.yt_stuff import YtConfig
import yatest.common


@pytest.fixture(scope="module")
def yt_config():
    return YtConfig(local_cypress_dir=yatest.common.work_path("local_cypress_tree"))


@pytest.fixture(scope="module")
def squeeze_bin_file():
    return yatest.common.binary_path("tools/mstand/squeeze_lib/bin/bin")
