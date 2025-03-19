import pytest

import yatest.common as common
import cloud.storage.core.tools.testing.fio.lib as fio

from cloud.filestore.tests.python.lib.common import get_nfs_mount_path


TESTS = fio.generate_tests()


@pytest.mark.parametrize("name", TESTS.keys())
def test_fio(name):
    mount_dir = get_nfs_mount_path()
    file_name = fio.get_file_name(mount_dir, name)

    results_path = common.output_path() + "/results-{}.txt".format(name)
    with open(results_path, 'w') as results:
        out = fio.run_test(file_name, TESTS[name])
        results.write(out)

    ret = common.canonical_file(results_path)
    return ret
