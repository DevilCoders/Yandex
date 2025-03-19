import pytest

from cloud.blockstore.tests.python.lib.test_with_plugin import run_plugin_test


class TestCase(object):

    def __init__(
            self,
            name,
            disk_id,
            test_config,
            host_major=0,
            host_minor=0,
            restart_interval=None,
            run_count=1):

        self.name = name
        self.disk_id = disk_id
        self.test_config = test_config
        self.host_major = host_major
        self.host_minor = host_minor
        self.restart_interval = restart_interval
        self.run_count = run_count


TESTS = [
    TestCase(
        "mount_rw",
        "vol0",
        "cloud/blockstore/tests/plugin/tcp/mount_rw.txt",
    ),
    TestCase(
        "remount",
        "vol0",
        "cloud/blockstore/tests/plugin/tcp/remount.txt",
    ),
    TestCase(
        "mount2",
        "vol0",
        "cloud/blockstore/tests/plugin/tcp/mount2.txt",
        5,
        3
    ),
    TestCase(
        "restarts",
        "vol0",
        "cloud/blockstore/tests/plugin/tcp/mount_rw.txt",
        restart_interval=3,
        run_count=500,
    ),
]


@pytest.mark.parametrize("test_case", TESTS, ids=[x.name for x in TESTS])
@pytest.mark.parametrize("plugin_version", ["trunk", "stable"])
def test_load(test_case, plugin_version):
    return run_plugin_test(
        test_case.name,
        test_case.disk_id,
        test_case.test_config,
        host_major=test_case.host_major,
        host_minor=test_case.host_minor,
        plugin_version=plugin_version,
        restart_interval=test_case.restart_interval,
        run_count=test_case.run_count)
