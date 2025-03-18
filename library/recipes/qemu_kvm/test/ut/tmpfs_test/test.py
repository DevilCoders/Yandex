import yatest.common
import os

import pytest


@pytest.mark.skipif(not yatest.common.context.flags.get('AUTOCHECK'), reason='only for distbuild')
def test_run_in_qemu_works():
    assert yatest.common.ram_drive_path()
    with open(os.path.join(yatest.common.ram_drive_path(), "tmpfs_checker"), 'w') as af:
        af.write("hello")

    with open(os.path.join(yatest.common.ram_drive_path(), "tmpfs_checker"), 'r') as af:
        assert "hello" in af.read()
