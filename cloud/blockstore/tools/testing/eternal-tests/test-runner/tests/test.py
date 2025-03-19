import pytest
import subprocess

import yatest.common as common


@pytest.mark.parametrize('cluster', ['hw-nbs-stable-lab', 'preprod', 'prod'])
@pytest.mark.parametrize('test_case', [
    'eternal-4tb',
    'eternal-1023gb-nonrepl',
    'eternal-640gb-verify-checkpoint'])
@pytest.mark.parametrize('command', [
    'setup-test',
    'stop-load',
    'rerun-load',
    'delete-test',
    'continue-load',
    'add-auto-run'])
def test_eternal_load_test(cluster, test_case, command):
    binary = common.binary_path(
        'cloud/blockstore/tools/testing/eternal-tests/test-runner/yc-nbs-run-eternal-load-tests')

    results_path = f'{common.output_path()}_results.txt'

    with open(results_path, 'w') as out:
        options = [
            binary,
            '--dry-run',
            command,
            '--cluster', cluster,
            '--test-case', test_case
        ]
        if command == 'run-test':
            options.append('--placement-group-name')
            options.append('placement-group')
        elif command == 'rerun-load' or command == 'add-auto-run':
            options.append('--write-rate')
            options.append('50')

        result = subprocess.call(options, stdout=out)
        assert result == 0

    ret = common.canonical_file(results_path)

    return ret


@pytest.mark.parametrize('cluster', ['hw-nbs-stable-lab', 'preprod', 'prod'])
@pytest.mark.parametrize('test_case', [
    'eternal-1tb-postgresql',
    'eternal-1023gb-nonrepl-mysql'])
def test_test_eternal_load_db_test(cluster, test_case):
    binary = common.binary_path(
        'cloud/blockstore/tools/testing/eternal-tests/test-runner/yc-nbs-run-eternal-load-tests')

    results_path = f'{common.output_path()}_results.txt'

    with open(results_path, 'w') as out:
        result = subprocess.call(
            [
                binary,
                '--dry-run',
                'rerun-db-load',
                '--cluster', cluster,
                '--test-case', test_case
            ],
            stdout=out
        )

        assert result == 0

    ret = common.canonical_file(results_path)

    return ret
