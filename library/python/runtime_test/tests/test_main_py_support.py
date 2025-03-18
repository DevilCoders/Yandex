import yatest.common as yc

import subprocess


def run_prog(path):
    return subprocess.check_output([yc.binary_path(path)]).strip()


def test_prog2():
    assert run_prog('library/python/runtime_test/py2_prog/py2_prog') == '1'


def test_prog3():
    assert run_prog('library/python/runtime_test/py3_prog/py3_prog') == '1'
