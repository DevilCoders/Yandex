import pytest
import subprocess

import yatest.common as common


@pytest.mark.parametrize('cluster', ['hw-nbs-stable-lab', 'preprod'])
def test_migration_test(cluster):
    binary = common.binary_path(
        'cloud/blockstore/tools/ci/migration_test/yc-nbs-ci-migration-test')

    results_path = '%s_results.txt' % common.output_path()

    with open(results_path, 'w') as out:
        result = subprocess.call(
            [
                binary,
                '--dry-run',
                '--teamcity',
                '--disk-name', 'fake-disk-id',
                '--kill-tablet',
                '--service-account-id', 'id',
                '--kill-period', '0',
                '--cluster', cluster
            ],
            stdout=out
        )

        assert result == 0

    ret = common.canonical_file(results_path)

    return ret
