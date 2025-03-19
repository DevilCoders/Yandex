import pytest
import subprocess

import yatest.common as common


@pytest.mark.parametrize('cluster', ['hw-nbs-stable-lab', 'prod'])
def test_checkpoint_validation_test(cluster):
    binary = common.binary_path(
        'cloud/blockstore/tools/ci/check_emptiness_test/yc-nbs-ci-check-nrd-disk-emptiness-test')

    results_path = '%s_results.txt' % common.output_path()

    with open(results_path, 'w') as out:
        result = subprocess.call(
            [
                binary,
                '--dry-run',
                '--teamcity',
                '--cluster', cluster,
                '--verify-test-path', 'verify/path'
            ],
            stdout=out
        )

        assert result == 0

    ret = common.canonical_file(results_path)

    return ret
