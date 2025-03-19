import pytest

import yatest.common as common
import cloud.storage.core.tools.testing.fio.lib as fio

from cloud.blockstore.tests.python.lib.test_base import get_nbs_device_path, get_file_size


KB = 1024
MB = 1024*KB

DEVICE_SIZE = 128*MB
TESTS = fio.generate_tests(size=DEVICE_SIZE, duration=60)


@pytest.mark.parametrize("name", TESTS.keys())
def test_fio(name):
    device_path = get_nbs_device_path()

    device_size = get_file_size(device_path)
    if DEVICE_SIZE != device_size:
        raise RuntimeError("Invalid device size (expected: {}, actual: {})".format(
            DEVICE_SIZE,
            device_size))

    results_path = common.output_path() + "/results-{}.txt".format(name)
    with open(results_path, 'w') as results:
        out = fio.run_test(device_path, TESTS[name])
        results.write(out)

    ret = common.canonical_file(results_path)
    return ret
