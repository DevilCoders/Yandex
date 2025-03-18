import yatest.common as yc
import subprocess


def test_with_enabled():
    path = yc.binary_path('library/cpp/malloc/calloc/tests/do_with_enabled/do_with_enabled')
    retcode = subprocess.call([path], stderr=subprocess.STDOUT, shell=False)
    assert retcode == 0


def test_with_disabled():
    path = yc.binary_path('library/cpp/malloc/calloc/tests/do_with_disabled/do_with_disabled')
    retcode = subprocess.call([path], stderr=subprocess.STDOUT, shell=False)
    assert retcode == 0


def test_with_lf():
    path = yc.binary_path('library/cpp/malloc/calloc/tests/do_with_lf/do_with_lf')
    retcode = subprocess.call([path], stderr=subprocess.STDOUT, shell=False)
    assert retcode == 0
