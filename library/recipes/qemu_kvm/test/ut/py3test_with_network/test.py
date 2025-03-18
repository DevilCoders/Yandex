import sys


def test():
    assert sys.version_info[0] == 3


def test_network_access(run):
    run(['nslookup', 'dist.yandex.ru'])
    run(['curl', 'dist.yandex.ru'])
