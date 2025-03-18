import subprocess

import yatest.common as yc


def test_at_fork():
    path = yc.binary_path('library/python/guid/at_fork_test/at_fork_test')

    assert len(frozenset(subprocess.check_output([path]).decode('utf-8').strip().split('\n'))) == 7
