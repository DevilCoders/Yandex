import os
import json

import pytest

from clickhouse.client import connect, BaseCursor

try:
    # Arcadia testing stuff
    import yatest.common as yc
except ImportError:
    yc = None

ARCADIA_PREFIX = 'library/python/clickhouse_client/tests/'


def open_file(filename, **kwargs):
    if yc is not None:
        path = yc.source_path(ARCADIA_PREFIX + filename)
    else:
        dirname = os.path.dirname(os.path.abspath(__file__))
        path = os.path.join(dirname, filename)
    return open(path, **kwargs)

def conn_config():
    return json.load(open_file('config/conn_params.json'))


@pytest.fixture(scope='session', name='conn_config')
def conn_config_fixture():
    return conn_config()


@pytest.fixture(scope='session')
def test_data():
    return json.load(open_file('config/test_data.json'))


def connection():
    return connect(**conn_config())


@pytest.fixture(scope='function', name='connection')
def connection_fixture():
    return connection()


def base_cursor_connection():
    return connect(cursor_class=BaseCursor, **conn_config())


@pytest.fixture(scope='function')
def base_cursor():
    cursor = base_cursor_connection().cursor()
    return cursor


@pytest.fixture(scope='function')
def cursor(test_data):
    cursor = connection().cursor()
    cursor._response_content = test_data[:]
    return cursor
