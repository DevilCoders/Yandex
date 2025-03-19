import pytest

import yatest.common as common
import cloud.storage.core.tools.testing.fio.lib as fio

from cloud.blockstore.tests.python.lib.test_base import get_nbs_device_path


TESTS = fio.generate_tests(verify=False)


@pytest.mark.parametrize("name", TESTS.keys())
def test_fio(name):
    device_path = get_nbs_device_path()

    results_path = common.output_path() + "/results-{}.txt".format(name)
    with open(results_path, 'w') as results:
        out = fio.run_test(device_path, TESTS[name])
        results.write(out)

    ret = common.canonical_file(results_path)
    return ret
