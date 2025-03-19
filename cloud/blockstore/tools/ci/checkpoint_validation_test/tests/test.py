import pytest
import subprocess

import yatest.common as common


@pytest.mark.parametrize('cluster', ['hw-nbs-stable-lab', 'preprod'])
def test_checkpoint_validation_test(cluster):
    binary = common.binary_path(
        'cloud/blockstore/tools/ci/checkpoint_validation_test/yc-nbs-ci-checkpoint-validation-test')

    results_path = '%s_results.txt' % common.output_path()

    with open(results_path, 'w') as out:
        result = subprocess.call(
            [
                binary,
                '--dry-run',
                '--teamcity',
                '--cluster', cluster,
                '--validator-path', 'validator/path',
                '--service-account-id', 'id'
            ],
            stdout=out
        )

        assert result == 0

    ret = common.canonical_file(results_path)

    return ret
