from __future__ import absolute_import
import os
import yaml
import pytest

try:
    # Arcadia testing stuff
    import yatest.common as yc
except ImportError:
    yc = None

ARCADIA_PREFIX = 'library/python/statface_client/tests/common/'


def open_file(filename, **kwargs):
    if yc is not None:
        path = yc.source_path(ARCADIA_PREFIX + filename)
    else:
        dirname = os.path.dirname(os.path.abspath(__file__))
        path = os.path.join(dirname, filename)
    return open(path, **kwargs)


@pytest.fixture
def config_yaml():
    return open_file('config.yaml').read()


@pytest.fixture
def user_config_yaml():
    return open_file('user_config.yaml').read()


@pytest.fixture
def report_example_tskv():
    return open_file('report_example.tskv').read()


@pytest.fixture
def buggy_yaml():
    return open_file('buggy_config.yaml').read()


@pytest.fixture
def statface_conf_yaml():
    return yaml.safe_load(open_file('statface_config.yaml'))
