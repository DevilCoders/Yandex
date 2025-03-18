import json

import responses
import pytest

from clickhouse.client import Cursor, BaseCursor
from clickhouse.tools import tsv_dump
from clickhouse.errors import ProgrammingError, OperationalError

from conftest import connect, conn_config, test_data


def test_rowcount(cursor, test_data):
    assert cursor.rowcount == len(test_data)


def test_fetchall(cursor, test_data):
    assert cursor.fetchall() == test_data


def test_fetchone(cursor, test_data):
    assert cursor.fetchone() == test_data[0]


def test_fetchmany(cursor, test_data):
    assert cursor.fetchmany() == [test_data[0]]


def test_fetchall_after_fetchone(cursor, test_data):
    _ = cursor.fetchone()
    assert cursor.fetchall() == test_data[1:]


def test_fetchone_after_fetchall(cursor):
    _ = cursor.fetchall()
    assert cursor.fetchone() is None


def test_fetchall_after_fetchall(cursor):
    _ = cursor.fetchall()
    assert cursor.fetchall() == []


def test_fetchmany_after_fetchall(cursor):
    _ = cursor.fetchall()
    assert cursor.fetchmany() == []


def test_fetchmany_with_custom_arraysize(cursor, test_data):
    cursor.arraysize = 2
    assert cursor.fetchmany() == test_data[:2]


def test_fetchmany_with_custom_size(cursor, test_data):
    assert cursor.fetchmany(2) == test_data[:2]


@responses.activate
def test_execute(test_data):
    cursor = connect(**conn_config()).cursor()
    data = test_data
    responses.add(
        responses.POST,
        cursor.connection.server_uri,
        body=json.dumps({'data': data}),
        status=200,
        content_type='application/json'
    )

    cursor.execute('test')
    assert cursor.fetchall() == data


@responses.activate
def test_execute_base_cursor(test_data):
    cursor = connect(cursor_class=BaseCursor, **conn_config()).cursor()
    csv_data = tsv_dump(["field_one", "field_two"] + test_data)
    responses.add(
        responses.POST,
        cursor.connection.server_uri,
        body=csv_data,
        status=200,
    )

    cursor.execute('test')
    assert cursor.fetchall() == csv_data


@responses.activate
def test_execute_fail():
    for cursor_class in (BaseCursor, Cursor):
        cursor = connect(cursor_class=cursor_class, **conn_config()).cursor()
        responses.add(
            responses.POST,
            cursor.connection.server_uri,
            body='we have a problem',
            status=500
        )
        with pytest.raises(OperationalError):
            cursor.execute('test')


def test_deny_formatting(cursor, base_cursor):
    bad_queries = (
        'select something from somewhere format json',
        'SELECT something FROM somewhere FORMAT JSON',
        'select something from somewhere where (something.id = 42)format json',
        'SELECT something FROM somewhere WHERE (something.id = 42)FORMAT JSON'
    )
    for query in bad_queries:
        with pytest.raises(OperationalError):
            base_cursor.execute(query)
        with pytest.raises(ProgrammingError):
            cursor.execute(query)


@responses.activate
def test_sheilded_error_on():
    for cursor_class in (BaseCursor, Cursor):
        config = conn_config()
        config.update(shielded_errors=(47,))
        cursor = connect(cursor_class=cursor_class, **config).cursor()
        responses.add(
            responses.POST,
            cursor.connection.server_uri,
            body='Code: 47. DB::Exception: Received from clickhouse. DB::Exception: Unknown identifier: from.',
            status=500
        )
        cursor.execute('select from')


@responses.activate
def test_sheilded_error_off():
    for cursor_class in (BaseCursor, Cursor):
        cursor = connect(cursor_class=cursor_class, **conn_config()).cursor()
        responses.add(
            responses.POST,
            cursor.connection.server_uri,
            body='Code: 47. DB::Exception: Received from clickhouse. DB::Exception: Unknown identifier: from.',
            status=500
        )
        with pytest.raises(OperationalError):
            cursor.execute('select from')
