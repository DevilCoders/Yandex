import pytest

from clickhouse.client import connect, Cursor
from clickhouse.errors import InternalError


def test_connection_args(conn_config, connection):
    for key, value in conn_config.items():
        assert value == getattr(connection, key)


def test_server_uri(conn_config, connection):
    uri = 'http://{host}:{port}'.format(**conn_config)
    assert uri == connection.server_uri


def test_cursor(connection):
    cursor = connection.cursor()
    assert isinstance(cursor, Cursor)
    assert cursor.connection == connection


def test_close_and_execute(connection):
    cursor = connection.cursor()
    connection.close()
    with pytest.raises(InternalError):
        cursor.execute('SELECT something FROM somewhere')


def test_close_and_get_cursor(connection):
    connection.close()
    with pytest.raises(InternalError):
        _ = connection.cursor()
