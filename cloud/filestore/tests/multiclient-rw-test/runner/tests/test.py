import pytest
import subprocess

import yatest.common as common


@pytest.mark.parametrize('cluster', ['hw-nbs-stable-lab', 'preprod', 'prod'])
def test_multiclient_rw_test(cluster):
    binary = common.binary_path(
        'cloud/filestore/tests/multiclient-rw-test/runner/yc-nfs-multiclient-test')

    results_path = '%s/%s_results.txt' % (common.output_path(), cluster)

    with open(results_path, 'w') as out:
        result = subprocess.call(
            [
                binary,
                '--dry-run',
                '--cluster', cluster,
                '--pairs', '1',
                '--scp-load-binary'
            ],
            stdout=out
        )

        assert result == 0

    ret = common.canonical_file(results_path)

    return ret
