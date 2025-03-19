# -*- coding: utf-8 -*-
"""
Metadb helpers tests
"""

import psycopg2
import pytest

from cloud.mdb.dbaas_worker.internal.metadb import DatabaseConnectionError, get_master_conn


def test_conn_closed_on_replicas(mocker):
    """
    Check that get_master_conn closes conns on replicas
    """
    connect = mocker.patch('psycopg2.connect')
    cursor = connect.return_value.cursor.return_value
    cursor.fetchone.return_value = {'transaction_read_only': 'on'}
    with pytest.raises(DatabaseConnectionError):
        get_master_conn('myprefix', ['host1', 'host2', 'host3'], mocker.MagicMock())
    assert connect.return_value.close.call_count == 3, 'All connections closed'


def test_conn_closed_on_error(mocker):
    """
    Check that get_master_conn closes conns on errors
    """
    connect = mocker.patch('psycopg2.connect')
    connect.return_value.cursor.side_effect = psycopg2.Error('test')
    connect.return_value.closed = 0
    with pytest.raises(DatabaseConnectionError):
        get_master_conn('myprefix', ['host1', 'host2', 'host3'], mocker.MagicMock())
    assert connect.return_value.close.call_count == 3, 'All connections closed'


def test_get_master_success(mocker):
    """
    Check that get_master_conn returns master conn
    """
    connect = mocker.patch('psycopg2.connect')
    cursor = connect.return_value.cursor.return_value
    cursor.fetchone.return_value = {'transaction_read_only': 'off'}
    res = get_master_conn('myprefix', ['host1'], mocker.MagicMock())
    assert res == connect.return_value, 'Unexpected return'


def test_orphan_conn_is_not_fatal(mocker):
    """
    Check than connection close errors are ignored
    """
    connect = mocker.patch('psycopg2.connect')
    connect.return_value.cursor.side_effect = psycopg2.Error('test')
    connect.return_value.closed = 0
    connect.return_value.close.side_effect = psycopg2.Error('test')
    with pytest.raises(DatabaseConnectionError):
        get_master_conn('myprefix', ['host1'], mocker.MagicMock())


def test_conn_error_is_not_fatal(mocker):
    """
    Check than connection close errors are ignored
    """
    connect = mocker.patch('psycopg2.connect')
    connect.side_effect = psycopg2.Error('test')
    with pytest.raises(DatabaseConnectionError):
        get_master_conn('myprefix', ['host1'], mocker.MagicMock())
