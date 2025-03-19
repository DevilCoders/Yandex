import datetime

import pytest
from library.python import resource

from cloud.dwh.test_utils.yt.misc import create_map_node
from cloud.dwh.test_utils.yt.misc import create_table
from cloud.dwh.test_utils.yt.misc import parse_yql_query_result
from cloud.dwh.test_utils.yt.misc import parse_yql_query_schema
from cloud.dwh.test_utils.yt.misc import run_yql_query


@pytest.fixture(scope='session')
def tables_sql():
    return 'tables.sql', resource.find('yql/utils/tables.sql')


@pytest.mark.parametrize('folder, path, expected', [
    ('//', 'table', '//table'),
    ('//dir', 'table', '//dir/table'),
    ('//dir', '/table', '//dir/table'),
    ('//dir/', 'table', '//dir/table'),
    ('//dir/', '/table', '//dir/table'),
    ('//dir/subdir', 'subdir2/table', '//dir/subdir/subdir2/table'),
])
def test_concat_path(yql_client, yt_client, tables_sql, folder, path, expected):
    query = f'''
    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $concat_path;

    SELECT $concat_path('{folder}', '{path}') as res
    '''

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['DataType', 'String']}
    assert result == [{'res': expected}]


def test_get_max_daily_table_path_not_existing_dir(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $get_max_daily_table_path;

    SELECT * FROM $get_max_daily_table_path('{test_prefix}/dir', 'plato')
    '''

    create_table(yt_client, path='dir1/dir2/2020-01-01', schema={'x': 'int32'}, data=[{'x': 1}])

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'path': ['OptionalType', ['DataType', 'String']]}
    assert result == [{'path': None}]


def test_get_max_daily_table_path_empty_dir(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $get_max_daily_table_path;

    SELECT * FROM $get_max_daily_table_path('{test_prefix}/dir', 'plato')
    '''

    create_map_node(yt_client, path='dir/1d')

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'path': ['OptionalType', ['DataType', 'String']]}
    assert result == [{'path': None}]


def test_get_max_daily_table_path_empty_table(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $get_max_daily_table_path;

    SELECT * FROM $get_max_daily_table_path('{test_prefix}/dir', 'plato')
    '''

    create_table(yt_client, path='dir/1d/2020-01-01', schema={'x': 'int32'}, data=[])

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'path': ['OptionalType', ['DataType', 'String']]}
    assert result == [{'path': None}]


def test_get_max_daily_table_path_3_tables(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $get_max_daily_table_path;

    SELECT * FROM $get_max_daily_table_path('{test_prefix}/dir', 'plato')
    '''

    create_table(yt_client, path='dir/1d/2020-01-02', schema={'x': 'int32'}, data=[{'x': 1}])
    create_table(yt_client, path='dir/1d/2020-02-01', schema={'x': 'int32'}, data=[{'x': 0}])
    create_table(yt_client, path='dir/1d/2020-01-01', schema={'x': 'int32'}, data=[{'x': 1}])

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'path': ['OptionalType', ['DataType', 'String']]}
    assert result == [{'path': f'{test_prefix}/dir/1d/2020-02-01'}]


def test_get_max_daily_table_path_3_tables_1_empty(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $get_max_daily_table_path;

    SELECT * FROM $get_max_daily_table_path('{test_prefix}/dir', 'plato')
    '''

    create_table(yt_client, path='dir/1d/2020-01-02', schema={'x': 'int32'}, data=[{'x': 1}])
    create_table(yt_client, path='dir/1d/2020-02-01', schema={'x': 'int32'}, data=[])
    create_table(yt_client, path='dir/1d/2020-01-01', schema={'x': 'int32'}, data=[{'x': 1}])

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'path': ['OptionalType', ['DataType', 'String']]}
    assert result == [{'path': f'{test_prefix}/dir/1d/2020-01-02'}]


def test_get_max_daily_table_path_2_tables_1_dir(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $get_max_daily_table_path;

    SELECT * FROM $get_max_daily_table_path('{test_prefix}/dir', 'plato')
    '''

    create_table(yt_client, path='dir/1d/2020-01-02', schema={'x': 'int32'}, data=[{'x': 1}])
    create_map_node(yt_client, path='dir/1d/2020-02-01/table')
    create_table(yt_client, path='dir/1d/2020-01-01', schema={'x': 'int32'}, data=[{'x': 1}])

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'path': ['OptionalType', ['DataType', 'String']]}
    assert result == [{'path': f'{test_prefix}/dir/1d/2020-01-02'}]


def test_select_last_not_empty_table_not_exists(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $select_last_not_empty_table;

    SELECT * FROM $select_last_not_empty_table('{test_prefix}/dir/1d', 'plato')
    '''

    create_map_node(yt_client, path='dir')

    with pytest.raises(AssertionError) as e:
        run_yql_query(yql_client, query, files=[tables_sql])

    assert 'Failed to get atom from an empty optional' in str(e.value)


def test_select_last_not_empty_table_empty_table(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $select_last_not_empty_table;

    SELECT * FROM $select_last_not_empty_table('{test_prefix}/dir/1d', 'plato')
    '''

    create_table(yt_client, path='dir/1d/2020-01-02', schema={'x': 'int32'}, data=[])

    with pytest.raises(AssertionError) as e:
        run_yql_query(yql_client, query, files=[tables_sql])

    assert 'Failed to get atom from an empty optional' in str(e.value)


def test_select_last_not_empty_table_3_tables(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $select_last_not_empty_table;

    SELECT * FROM $select_last_not_empty_table('{test_prefix}/dir/1d', 'plato')
    '''

    create_table(yt_client, path='dir/1d/2020-01-02', schema={'x': 'int32'}, data=[{'x': 3}])
    create_table(yt_client, path='dir/1d/2020-02-01', schema={'x': 'int32'}, data=[{'x': 0}])
    create_table(yt_client, path='dir/1d/2020-01-01', schema={'x': 'int32'}, data=[{'x': 2}])

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)

    assert result == [{'x': 0}]


def test_select_last_not_empty_table_3_tables_1_empty(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $select_last_not_empty_table;

    SELECT * FROM $select_last_not_empty_table('{test_prefix}/dir/1d', 'plato')
    '''

    create_table(yt_client, path='dir/1d/2020-01-02', schema={'x': 'int32'}, data=[{'x': 3}])
    create_table(yt_client, path='dir/1d/2020-02-01', schema={'x': 'int32'}, data=[])
    create_table(yt_client, path='dir/1d/2020-01-01', schema={'x': 'int32'}, data=[{'x': 2}])

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)

    assert result == [{'x': 3}]


def test_select_last_not_empty_table_2_tables_1_dir(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $select_last_not_empty_table;

    SELECT * FROM $select_last_not_empty_table('{test_prefix}/dir/1d', 'plato')
    '''

    create_table(yt_client, path='dir/1d/2020-01-02', schema={'x': 'int32'}, data=[{'x': 3}])
    create_map_node(yt_client, path='dir/1d/2020-02-01/table')
    create_table(yt_client, path='dir/1d/2020-01-01', schema={'x': 'int32'}, data=[{'x': 2}])

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)

    assert result == [{'x': 3}]


def test_get_max_date_from_table_path_not_exists(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $get_max_date_from_table_path;

    SELECT * FROM $get_max_date_from_table_path('{test_prefix}/dir', 'plato')
    '''

    create_map_node(yt_client, path='dir')

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'max_date': ['OptionalType', ['DataType', 'Date']]}
    assert result == [{'max_date': None}]


def test_get_max_date_from_table_path_empty_table(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $get_max_date_from_table_path;

    SELECT * FROM $get_max_date_from_table_path('{test_prefix}/dir/1d', 'plato')
    '''

    create_table(yt_client, path='dir/1d/2020-02-01', schema={'x': 'int32'}, data=[])

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'max_date': ['OptionalType', ['DataType', 'Date']]}
    assert result == [{'max_date': None}]


def test_get_max_date_from_table_path_3_tables_1_empty(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $get_max_date_from_table_path;

    SELECT * FROM $get_max_date_from_table_path('{test_prefix}/dir/1d', 'plato')
    '''

    create_table(yt_client, path='dir/1d/2020-01-02', schema={'x': 'int32'}, data=[{'x': 3}])
    create_table(yt_client, path='dir/1d/2020-02-01', schema={'x': 'int32'}, data=[])
    create_table(yt_client, path='dir/1d/2020-01-01', schema={'x': 'int32'}, data=[{'x': 2}])

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'max_date': ['OptionalType', ['DataType', 'Date']]}
    assert result == [{'max_date': datetime.date(2020, 1, 2)}]


def test_get_max_date_from_table_path_3_tables(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $get_max_date_from_table_path;

    SELECT * FROM $get_max_date_from_table_path('{test_prefix}/dir/1d', 'plato')
    '''

    create_table(yt_client, path='dir/1d/2020-01-02', schema={'x': 'int32'}, data=[{'x': 3}])
    create_table(yt_client, path='dir/1d/2020-02-01', schema={'x': 'int32'}, data=[{'x': 0}])
    create_table(yt_client, path='dir/1d/2020-01-01', schema={'x': 'int32'}, data=[{'x': 2}])

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'max_date': ['OptionalType', ['DataType', 'Date']]}
    assert result == [{'max_date': datetime.date(2020, 2, 1)}]


def test_get_max_date_from_table_path_2_tables_1_dir(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $get_max_date_from_table_path;

    SELECT * FROM $get_max_date_from_table_path('{test_prefix}/dir/1d', 'plato')
    '''

    create_table(yt_client, path='dir/1d/2020-01-02', schema={'x': 'int32'}, data=[{'x': 3}])
    create_map_node(yt_client, path='dir/1d/2020-02-01/table')
    create_table(yt_client, path='dir/1d/2020-01-01', schema={'x': 'int32'}, data=[{'x': 2}])

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'max_date': ['OptionalType', ['DataType', 'Date']]}
    assert result == [{'max_date': datetime.date(2020, 1, 2)}]


def test_get_all_daily_tables_0_d_tables_0_h_tables(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $get_all_daily_tables;

    SELECT * FROM $get_all_daily_tables('{test_prefix}/dir', 'plato')
    '''

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'paths': ['ListType', ['DataType', 'String']]}
    assert result == [{'paths': []}]


def test_get_all_daily_tables_1_d_empty_3_h_tables(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $get_all_daily_tables;

    SELECT * FROM $get_all_daily_tables('{test_prefix}/dir', 'plato')
    '''

    create_table(yt_client, path='dir/1d/2020-01-02', schema={'x': 'int32'}, data=[])

    create_table(yt_client, path='dir/1h/2020-02-02T00:00:00', schema={'x': 'int32'}, data=[{'x': 3}])
    create_table(yt_client, path='dir/1h/2020-02-02T01:00:00', schema={'x': 'int32'}, data=[{'x': 3}])
    create_table(yt_client, path='dir/1h/2020-02-02T02:00:00', schema={'x': 'int32'}, data=[])
    create_table(yt_client, path='dir/1h/2020-02-03T00:00:00', schema={'x': 'int32'}, data=[{'x': 3}])

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'paths': ['ListType', ['DataType', 'String']]}

    expected = sorted([
        f'{test_prefix}/dir/1h/2020-02-02T00:00:00',
        f'{test_prefix}/dir/1h/2020-02-02T01:00:00',
        f'{test_prefix}/dir/1h/2020-02-03T00:00:00',
    ])
    result[0]['paths'].sort()
    assert result == [{'paths': expected}]


def test_get_all_daily_tables_3_d_table_1_h_empty(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $get_all_daily_tables;

    SELECT * FROM $get_all_daily_tables('{test_prefix}/dir', 'plato')
    '''

    create_table(yt_client, path='dir/1d/2020-01-03', schema={'x': 'int32'}, data=[])
    create_table(yt_client, path='dir/1d/2020-01-02', schema={'x': 'int32'}, data=[{'x': 3}])
    create_table(yt_client, path='dir/1d/2020-02-01', schema={'x': 'int32'}, data=[{'x': 0}])
    create_table(yt_client, path='dir/1d/2020-01-01', schema={'x': 'int32'}, data=[{'x': 2}])

    create_table(yt_client, path='dir/1h/2020-02-02T00:00:00', schema={'x': 'int32'}, data=[])
    create_table(yt_client, path='dir/1h/2020-02-02T01:00:00', schema={'x': 'int32'}, data=[])
    create_table(yt_client, path='dir/1h/2020-02-02T02:00:00', schema={'x': 'int32'}, data=[])
    create_table(yt_client, path='dir/1h/2020-02-03T00:00:00', schema={'x': 'int32'}, data=[])

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'paths': ['ListType', ['DataType', 'String']]}

    expected = sorted([
        f'{test_prefix}/dir/1d/2020-01-01',
        f'{test_prefix}/dir/1d/2020-01-02',
        f'{test_prefix}/dir/1d/2020-02-01',
    ])
    result[0]['paths'].sort()
    assert result == [{'paths': expected}]


def test_get_all_daily_tables_3_d_tables_0_h_tables(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $get_all_daily_tables;

    SELECT * FROM $get_all_daily_tables('{test_prefix}/dir', 'plato')
    '''

    create_table(yt_client, path='dir/1d/2020-01-03', schema={'x': 'int32'}, data=[])
    create_table(yt_client, path='dir/1d/2020-01-02', schema={'x': 'int32'}, data=[{'x': 3}])
    create_table(yt_client, path='dir/1d/2020-02-01', schema={'x': 'int32'}, data=[{'x': 0}])
    create_table(yt_client, path='dir/1d/2020-01-01', schema={'x': 'int32'}, data=[{'x': 2}])

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'paths': ['ListType', ['DataType', 'String']]}

    expected = sorted([
        f'{test_prefix}/dir/1d/2020-01-01',
        f'{test_prefix}/dir/1d/2020-01-02',
        f'{test_prefix}/dir/1d/2020-02-01',
    ])
    result[0]['paths'].sort()
    assert result == [{'paths': expected}]


def test_get_all_daily_tables_0_d_tables_3_h_tables(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $get_all_daily_tables;

    SELECT * FROM $get_all_daily_tables('{test_prefix}/dir', 'plato')
    '''

    create_table(yt_client, path='dir/1h/2020-02-02T00:00:00', schema={'x': 'int32'}, data=[{'x': 3}])
    create_table(yt_client, path='dir/1h/2020-02-02T01:00:00', schema={'x': 'int32'}, data=[{'x': 3}])
    create_table(yt_client, path='dir/1h/2020-02-02T02:00:00', schema={'x': 'int32'}, data=[])
    create_table(yt_client, path='dir/1h/2020-02-03T00:00:00', schema={'x': 'int32'}, data=[{'x': 3}])

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'paths': ['ListType', ['DataType', 'String']]}

    expected = sorted([
        f'{test_prefix}/dir/1h/2020-02-02T00:00:00',
        f'{test_prefix}/dir/1h/2020-02-02T01:00:00',
        f'{test_prefix}/dir/1h/2020-02-03T00:00:00',
    ])
    result[0]['paths'].sort()
    assert result == [{'paths': expected}]


def test_get_all_daily_tables_3_d_tables_3_h_tables(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $get_all_daily_tables;

    SELECT * FROM $get_all_daily_tables('{test_prefix}/dir', 'plato')
    '''

    create_table(yt_client, path='dir/1d/2020-01-03', schema={'x': 'int32'}, data=[])
    create_table(yt_client, path='dir/1d/2020-01-02', schema={'x': 'int32'}, data=[{'x': 3}])
    create_table(yt_client, path='dir/1d/2020-02-01', schema={'x': 'int32'}, data=[{'x': 0}])
    create_table(yt_client, path='dir/1d/2020-01-01', schema={'x': 'int32'}, data=[{'x': 2}])

    create_table(yt_client, path='dir/1h/2020-02-01T00:00:00', schema={'x': 'int32'}, data=[{'x': 3}])
    create_table(yt_client, path='dir/1h/2020-02-01T23:00:00', schema={'x': 'int32'}, data=[{'x': 3}])
    create_table(yt_client, path='dir/1h/2020-02-02T00:00:00', schema={'x': 'int32'}, data=[{'x': 3}])
    create_table(yt_client, path='dir/1h/2020-02-02T01:00:00', schema={'x': 'int32'}, data=[{'x': 3}])
    create_table(yt_client, path='dir/1h/2020-02-02T02:00:00', schema={'x': 'int32'}, data=[])
    create_table(yt_client, path='dir/1h/2020-02-03T00:00:00', schema={'x': 'int32'}, data=[{'x': 3}])

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'paths': ['ListType', ['DataType', 'String']]}

    expected = sorted([
        f'{test_prefix}/dir/1d/2020-01-01',
        f'{test_prefix}/dir/1d/2020-01-02',
        f'{test_prefix}/dir/1d/2020-02-01',
        f'{test_prefix}/dir/1h/2020-02-02T00:00:00',
        f'{test_prefix}/dir/1h/2020-02-02T01:00:00',
        f'{test_prefix}/dir/1h/2020-02-03T00:00:00',
    ])
    result[0]['paths'].sort()
    assert result == [{'paths': expected}]


@pytest.mark.parametrize('folder, table_name, table_to_create, expected', [
    ('dir/1d', 'table', 'dir/table', False),
    ('dir/1d', 'table', 'dir/1d/table1', False),
    ('dir/1d', 'subdir', 'dir/1d/subdir/table', False),
    ('dir/1d', 'table', 'dir/1d/table', True),
])
def test_is_table_exists(yql_client, yt_client, tables_sql, test_prefix, folder, table_name, table_to_create, expected):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $is_table_exists;

    SELECT * FROM $is_table_exists('{test_prefix}/{folder}', '{table_name}', 'plato')
    '''

    create_table(yt_client, path=table_to_create, schema={'x': 'int32'}, data=[])

    response = run_yql_query(yql_client, query, files=[tables_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'is_exists': ['DataType', 'Bool']}
    assert result == [{'is_exists': expected}]


def test_select_transfer_manager_table(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $select_transfer_manager_table;

    SELECT * FROM $select_transfer_manager_table('{test_prefix}/dir', 'plato')
    '''

    create_table(yt_client, path='dir/2019-01-01', schema={'x': 'int32'}, data=[{'x': 1}])
    create_table(yt_client, path='dir/2020-01-01', schema={'x': 'int32'}, data=[{'x': 2}])
    create_table(yt_client, path='dir/2021-01-01', schema={'x': 'int32'}, data=[])

    response = run_yql_query(yql_client, query, files=[tables_sql])
    result = parse_yql_query_result(response)

    assert result == [{'x': 1}]


def test_select_transfer_manager_table_empty_dir(yql_client, yt_client, tables_sql, test_prefix):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $select_transfer_manager_table;

    SELECT * FROM $select_transfer_manager_table('{test_prefix}/dir', 'plato')
    '''

    create_map_node(yt_client, path='dir/1d')

    with pytest.raises(AssertionError, match="Table name must not be empty"):
        run_yql_query(yql_client, query, files=[tables_sql])


@pytest.mark.parametrize('data, expected', [
    ([
        {"path": "dir/2019-01-01", "schema": {'x': 'int32'}, "data": [{'x': 1}]},
        {"path": "dir/2019-01-02", "schema": {'x': 'int32'}, "data": [{'x': 2}]},
        {"path": "dir/2019-01-03", "schema": {'x': 'int32'}, "data": []},
    ], "dir/2019-01-02"),
    ([
        {"path": "dir/2019-01-01", "schema": {'x': 'int32'}, "data": [{'x': 1}]},
        {"path": "dir/2019-01-03", "schema": {'x': 'int32'}, "data": []},
    ], "dir/2019-01-01")
])
def test_get_all_non_empty_tables(yql_client, yt_client, tables_sql, test_prefix, data, expected):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $get_all_non_empty_tables;

    SELECT * FROM $get_all_non_empty_tables( '{test_prefix}/dir', 'plato')
    '''

    for table in data:
        create_table(yt_client, table['path'], table['schema'], table['data'])

    response = run_yql_query(yql_client, query, files=[tables_sql])
    result = parse_yql_query_result(response)

    assert result[0]['Path'] == test_prefix + '/' + expected


@pytest.mark.parametrize('source_data, target_data,  expected', [
    # case: when source and target empty shoudl retrun empty result
    ([], [], []),
    # case: when source has data and target does not shoudl retrun all tables from source
    ([
        {"path": "src/2019-01-01", "schema": {'x': 'int32'}, "data": [{'x': 1}]},
        {"path": "src/2019-01-02", "schema": {'x': 'int32'}, "data": [{'x': 2}]}
    ], [], ["2019-01-02", "2019-01-01"]),
    # case: when source and target have table names should retrun tables that are not in target
    ([
        {"path": "src/2019-01-01", "schema": {'x': 'int32'}, "data": [{'x': 1}]},
        {"path": "src/2019-01-02", "schema": {'x': 'int32'}, "data": [{'x': 2}]},
    ],
        [
        {"path": "dst/2019-01-01", "schema": {'x': 'int32'}, "data": [{'x': 1}]},
    ], ["2019-01-02"]),
    # case: when source and target have the same table names should return empty result
    ([
        {"path": "src/2019-01-01", "schema": {'x': 'int32'}, "data": [{'x': 1}]},
        {"path": "src/2019-01-02", "schema": {'x': 'int32'}, "data": [{'x': 2}]},
    ],
        [
        {"path": "dst/2019-01-01", "schema": {'x': 'int32'}, "data": [{'x': 1}]},
        {"path": "dst/2019-01-02", "schema": {'x': 'int32'}, "data": [{'x': 2}]},
    ], [])
])
def test_get_source_tables_that_not_in_target(yql_client, yt_client, tables_sql, test_prefix, source_data, target_data, expected):
    query = f'''
    USE plato;

    PRAGMA Library('tables.sql');
    IMPORT `tables` SYMBOLS $get_source_tables_that_not_in_target;

    SELECT * FROM $get_source_tables_that_not_in_target('{test_prefix}/src', '{test_prefix}/dst', 'plato')
    '''

    for table in source_data:
        create_table(yt_client, table['path'], table['schema'], table['data'])

    for table in target_data:
        create_table(yt_client, table['path'], table['schema'], table['data'])

    response = run_yql_query(yql_client, query, files=[tables_sql])
    result = parse_yql_query_result(response)

    assert result[0]['tables'] == expected
