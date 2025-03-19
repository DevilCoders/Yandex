import pytest
import subprocess

import yatest.common as common


@pytest.mark.parametrize("test_case", ["nfs", "nbs"])
@pytest.mark.parametrize("cluster", ["hw-nbs-stable-lab", "preprod", "prod"])
def test_arcadia_build_test(test_case, cluster):
    binary = common.binary_path(
        "cloud/filestore/tests/build-arcadia-test/yc-nfs-ci-build-arcadia-test")

    results_path = "%s/%s_results.txt" % (common.output_path(), test_case)

    with open(results_path, "w") as out:
        result = subprocess.call(
            [
                binary,
                "--dry-run",
                "--teamcity",
                "--cluster", cluster,
                "--test-case", test_case
            ],
            stdout=out
        )

        assert result == 0

    ret = common.canonical_file(results_path)

    return ret
