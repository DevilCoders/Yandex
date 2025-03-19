import datetime

import pytest
from library.python import resource

from cloud.dwh.test_utils.yt.misc import parse_yql_query_result
from cloud.dwh.test_utils.yt.misc import parse_yql_query_schema
from cloud.dwh.test_utils.yt.misc import run_yql_query


@pytest.fixture(scope='session')
def datetime_sql():
    return 'datetime.sql', resource.find('yql/utils/datetime.sql')


def test_inf_constants(yql_client, datetime_sql):
    query = '''
    PRAGMA Library('datetime.sql');
    IMPORT `datetime` SYMBOLS $DATETIME_INF, $DATETIME_NEG_INF;

    SELECT $DATETIME_INF AS res_inf, $DATETIME_NEG_INF AS res_neg_inf
    '''

    response = run_yql_query(yql_client, query, files=[datetime_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    expected_result = [{'res_inf': datetime.datetime(2105, 12, 31, 23, 59, 59),
                        'res_neg_inf': datetime.datetime(1970, 1, 1, 0, 0)}]
    expected_schema = {'res_inf': ['OptionalType', ['DataType', 'Datetime']],
                       'res_neg_inf': ['OptionalType', ['DataType', 'Datetime']]}

    assert result == expected_result
    assert schema == expected_schema


def test_utc_timezone(yql_client, datetime_sql):
    query = '''
    PRAGMA Library('datetime.sql');
    IMPORT `datetime` SYMBOLS $UTC_TIMEZONE;

    SELECT $UTC_TIMEZONE as res
    '''

    response = run_yql_query(yql_client, query, files=[datetime_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['DataType', 'String']}
    assert result == [{'res': 'UTC'}]


def test_msk_timezone(yql_client, datetime_sql):
    query = '''
    PRAGMA Library('datetime.sql');
    IMPORT `datetime` SYMBOLS $MSK_TIMEZONE;

    SELECT $MSK_TIMEZONE as res
    '''

    response = run_yql_query(yql_client, query, files=[datetime_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['DataType', 'String']}
    assert result == [{'res': 'Europe/Moscow'}]


def test_get_timestamp_from_days(yql_client, datetime_sql):
    query = '''
    PRAGMA Library('datetime.sql');
    IMPORT `datetime` SYMBOLS $get_timestamp_from_days;

    SELECT $get_timestamp_from_days(1) as res
    '''

    response = run_yql_query(yql_client, query, files=[datetime_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['DataType', 'Int32']}
    assert result == [{'res': 24 * 60 * 60}]


@pytest.mark.parametrize('func, dttm, expected', [
    ('$format_date', '2021-02-03T04:05:06Z', '2021-02-03'),
    ('$format_datetime', '2021-02-03T04:05:06Z', '2021-02-03 04:05:06'),
    ('$format_time', '2021-02-03T04:05:06Z', '04:05:06'),
    ('$format_iso8601', '2021-02-03T04:05:06Z', '2021-02-03T04:05:06+0000'),
    ('$format_hour', '2021-02-03T04:05:06Z', '2021-02-03T04:00:00'),
    ('$format_month', '2021-02-03T04:05:06Z', '2021-02-01'),
    ('$format_month_cohort', '2021-02-03T04:05:06Z', '2021-02'),
    ('$format_month_name', '2021-02-03T04:05:06Z', 'February'),
    ('$format_year', '2021-02-03T04:05:06Z', '2021'),
    ('$format_quarter', '2021-01-03T04:05:06Z', '2021-Q1'),
    ('$format_quarter', '2021-02-03T04:05:06Z', '2021-Q1'),
    ('$format_quarter', '2021-03-03T04:05:06Z', '2021-Q1'),
    ('$format_quarter', '2021-04-03T04:05:06Z', '2021-Q2'),
    ('$format_quarter', '2021-05-03T04:05:06Z', '2021-Q2'),
    ('$format_quarter', '2021-06-03T04:05:06Z', '2021-Q2'),
    ('$format_quarter', '2021-07-03T04:05:06Z', '2021-Q3'),
    ('$format_quarter', '2021-08-03T04:05:06Z', '2021-Q3'),
    ('$format_quarter', '2021-09-03T04:05:06Z', '2021-Q3'),
    ('$format_quarter', '2021-10-03T04:05:06Z', '2021-Q4'),
    ('$format_quarter', '2021-11-03T04:05:06Z', '2021-Q4'),
    ('$format_quarter', '2021-12-03T04:05:06Z', '2021-Q4'),
    ('$format_half_year', '2021-01-03T04:05:06Z', '2021-H1'),
    ('$format_half_year', '2021-02-03T04:05:06Z', '2021-H1'),
    ('$format_half_year', '2021-03-03T04:05:06Z', '2021-H1'),
    ('$format_half_year', '2021-04-03T04:05:06Z', '2021-H1'),
    ('$format_half_year', '2021-05-03T04:05:06Z', '2021-H1'),
    ('$format_half_year', '2021-06-03T04:05:06Z', '2021-H1'),
    ('$format_half_year', '2021-07-03T04:05:06Z', '2021-H2'),
    ('$format_half_year', '2021-08-03T04:05:06Z', '2021-H2'),
    ('$format_half_year', '2021-09-03T04:05:06Z', '2021-H2'),
    ('$format_half_year', '2021-10-03T04:05:06Z', '2021-H2'),
    ('$format_half_year', '2021-11-03T04:05:06Z', '2021-H2'),
    ('$format_half_year', '2021-12-03T04:05:06Z', '2021-H2'),
])
def test_format(yql_client, datetime_sql, func, dttm, expected):
    query = f'''
    PRAGMA Library('datetime.sql');
    IMPORT `datetime` SYMBOLS {func};

    SELECT {func}(Datetime("{dttm}")) as res
    '''

    response = run_yql_query(yql_client, query, files=[datetime_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['DataType', 'String']}
    assert result == [{'res': expected}]


@pytest.mark.parametrize('ts, tz, expected', [
    (1612314306, 'Asia/Yekaterinburg', '2021-02-03T06:05:06,Asia/Yekaterinburg'),
])
def test_get_tz_datetime(yql_client, datetime_sql, ts, tz, expected):
    query = f'''
    PRAGMA Library('datetime.sql');
    IMPORT `datetime` SYMBOLS $get_tz_datetime;

    SELECT $get_tz_datetime({ts}, '{tz}') as res
    '''

    response = run_yql_query(yql_client, query, files=[datetime_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['OptionalType', ['DataType', 'TzDatetime']]}
    assert result == [{'res': expected}]


@pytest.mark.parametrize('func, ts, expected', [
    ('$get_utc_datetime', 1612314306, '2021-02-03T01:05:06,UTC'),
    ('$get_msk_datetime', 1612314306, '2021-02-03T04:05:06,Europe/Moscow'),
])
def test_get_datetime_without_explicit_tz(yql_client, datetime_sql, func, ts, expected):
    query = f'''
    PRAGMA Library('datetime.sql');
    IMPORT `datetime` SYMBOLS {func};

    SELECT {func}({ts}) as res
    '''

    response = run_yql_query(yql_client, query, files=[datetime_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['OptionalType', ['DataType', 'TzDatetime']]}
    assert result == [{'res': expected}]


@pytest.mark.parametrize('ts, tz, fmt, expected', [
    (1612314306, 'UTC', '%Y-%m-%d', '2021-02-03'),
    (1612314306, 'UTC', '%Y-%m-%dT%H:%M:%S%z', '2021-02-03T01:05:06-0000'),
    (1612314306, 'Europe/Moscow', '%Y-%m-%dT%H:%M:%S%z', '2021-02-03T04:05:06+0300'),
])
def test_format_by_timestamp(yql_client, datetime_sql, ts, tz, fmt, expected):
    query = f'''
    PRAGMA Library('datetime.sql');
    IMPORT `datetime` SYMBOLS $format_by_timestamp;

    SELECT $format_by_timestamp({ts}, '{tz}', DateTime::Format('{fmt}')) as res
    '''

    response = run_yql_query(yql_client, query, files=[datetime_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['OptionalType', ['DataType', 'String']]}
    assert result == [{'res': expected}]


@pytest.mark.parametrize('func, ts, expected', [
    ('$format_utc_date_by_timestamp', 1612386306, '2021-02-03'),
    ('$format_msk_date_by_timestamp', 1612386306, '2021-02-04'),
    ('$format_utc_datetime_by_timestamp', 1612386306, '2021-02-03 21:05:06'),
    ('$format_msk_datetime_by_timestamp', 1612386306, '2021-02-04 00:05:06'),
    ('$format_utc_time_by_timestamp', 1612386306, '21:05:06'),
    ('$format_msk_time_by_timestamp', 1612386306, '00:05:06'),
    ('$format_utc_iso8601_by_timestamp', 1612386306, '2021-02-03T21:05:06-0000'),
    ('$format_msk_iso8601_by_timestamp', 1612386306, '2021-02-04T00:05:06+0300'),
    ('$format_utc_hour_by_timestamp', 1612386306, '2021-02-03T21:00:00'),
    ('$format_msk_hour_by_timestamp', 1612386306, '2021-02-04T00:00:00'),
    ('$format_utc_month_by_timestamp', 1609448706, '2020-12-01'),
    ('$format_msk_month_by_timestamp', 1612386306, '2021-02-01'),
    ('$format_utc_month_cohort_by_timestamp', 1609448706, '2020-12'),
    ('$format_msk_month_cohort_by_timestamp', 1612386306, '2021-02'),
    ('$format_utc_month_name_by_timestamp', 1609448706, 'December'),
    ('$format_msk_month_name_by_timestamp', 1609448706, 'January'),
    ('$format_utc_quarter_by_timestamp', 1609448706, '2020-Q4'),
    ('$format_msk_quarter_by_timestamp', 1609448706, '2021-Q1'),
    ('$format_utc_half_year_by_timestamp', 1609448706, '2020-H2'),
    ('$format_msk_half_year_by_timestamp', 1609448706, '2021-H1'),
    ('$format_utc_year_by_timestamp', 1609448706, '2020'),
    ('$format_msk_year_by_timestamp', 1609448706, '2021'),
])
def test_format_by_timestamp_functions_without_explicit_tz(yql_client, datetime_sql, func, ts, expected):
    query = f'''
    PRAGMA Library('datetime.sql');
    IMPORT `datetime` SYMBOLS {func};

    SELECT {func}({ts}) as res
    '''

    response = run_yql_query(yql_client, query, files=[datetime_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['OptionalType', ['DataType', 'String']]}
    assert result == [{'res': expected}]


@pytest.mark.parametrize('func, date_str, expected', [
    ('$parse_hour_date_format_to_ts_msk', '2021-02-03T21:00:00', '2021-02-03T21:00:00,Europe/Moscow'),
])
def test_parse_str_to_datetime_functions(yql_client, datetime_sql, func, date_str, expected):
    query = f'''
    PRAGMA Library('datetime.sql');
    IMPORT `datetime` SYMBOLS {func};

    SELECT {func}('{date_str}') as res
    '''

    response = run_yql_query(yql_client, query, files=[datetime_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['OptionalType', ['DataType', 'TzTimestamp']]}
    assert result == [{'res': expected}]


@pytest.mark.parametrize('dt, expected', [
    ('2021-02-03', '2021-02-04'),
    ('2020-02-28', '2020-02-29'),
    ('2021-02-28', '2021-03-01'),
])
def test_get_next_date(yql_client, datetime_sql, dt, expected):
    query = f'''
    PRAGMA Library('datetime.sql');
    IMPORT `datetime` SYMBOLS $get_next_date;

    SELECT $get_next_date('{dt}') as res
    '''

    response = run_yql_query(yql_client, query, files=[datetime_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['OptionalType', ['DataType', 'String']]}
    assert result == [{'res': expected}]


@pytest.mark.parametrize('start, end, expected', [
    ('2021-02-03', '2021-02-02', []),
    ('2021-02-03', '2021-02-04', ['2021-02-03', '2021-02-04']),
    ('2021-02-04', '2021-02-04', ['2021-02-04']),
    ('2020-02-27', '2020-03-02', ['2020-02-27', '2020-02-28', '2020-02-29', '2020-03-01', '2020-03-02']),
])
def test_get_date_range_inclusive_str_input(yql_client, datetime_sql, start, end, expected):
    query = f'''
    PRAGMA Library('datetime.sql');
    IMPORT `datetime` SYMBOLS $get_date_range_inclusive;

    SELECT $get_date_range_inclusive('{start}', '{end}') as res
    '''

    response = run_yql_query(yql_client, query, files=[datetime_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['ListType', ['OptionalType', ['DataType', 'String']]]}
    assert result == [{'res': expected}]


def test_get_date_range_inclusive_tz_dttm_input(yql_client, datetime_sql):
    query = '''
    PRAGMA Library('datetime.sql');
    IMPORT `datetime` SYMBOLS $get_date_range_inclusive;

    $start = TzDateTime('2021-01-02T02:05:06,Europe/Moscow');
    $end = TzDateTime('2021-01-03T02:05:06,Europe/Moscow');

    SELECT $get_date_range_inclusive($start, $end) as res
    '''

    response = run_yql_query(yql_client, query, files=[datetime_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['ListType', ['OptionalType', ['DataType', 'String']]]}
    assert result == [{'res': ['2021-01-02', '2021-01-03']}]


def test_get_date_range_inclusive_dttm_input(yql_client, datetime_sql):
    query = '''
    PRAGMA Library('datetime.sql');
    IMPORT `datetime` SYMBOLS $get_date_range_inclusive;

    $start = Datetime('2021-01-02T02:05:06Z');
    $end = Datetime('2021-01-03T03:05:06Z');

    SELECT $get_date_range_inclusive($start, $end) as res
    '''

    response = run_yql_query(yql_client, query, files=[datetime_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['ListType', ['OptionalType', ['DataType', 'String']]]}
    assert result == [{'res': ['2021-01-02', '2021-01-03']}]


@pytest.mark.parametrize('start, end, expected', [
    ('2021-02-03', '2021-02-02', []),
    ('2021-02-03', '2021-02-04', ['2021-02-01']),
    ('2021-02-04', '2021-02-04', ['2021-02-01']),
    ('2020-11-27', '2021-02-01', ['2020-11-01', '2020-12-01', '2021-01-01', '2021-02-01']),
])
def test_get_month_range_inclusive_str_input(yql_client, datetime_sql, start, end, expected):
    query = f'''
    PRAGMA Library('datetime.sql');
    IMPORT `datetime` SYMBOLS $get_month_range_inclusive;

    SELECT $get_month_range_inclusive('{start}', '{end}') as res
    '''

    response = run_yql_query(yql_client, query, files=[datetime_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['ListType', ['OptionalType', ['DataType', 'String']]]}
    assert result == [{'res': expected}]


def test_get_month_range_inclusive_tz_dttm_input(yql_client, datetime_sql):
    query = '''
    PRAGMA Library('datetime.sql');
    IMPORT `datetime` SYMBOLS $get_month_range_inclusive;

    $start = TzDateTime('2021-01-02T02:05:06,Europe/Moscow');
    $end = TzDateTime('2021-02-03T02:05:06,Europe/Moscow');

    SELECT $get_month_range_inclusive($start, $end) as res
    '''

    response = run_yql_query(yql_client, query, files=[datetime_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['ListType', ['OptionalType', ['DataType', 'String']]]}
    assert result == [{'res': ['2021-01-01', '2021-02-01']}]


def test_get_month_range_inclusive_dttm_input(yql_client, datetime_sql):
    query = '''
    PRAGMA Library('datetime.sql');
    IMPORT `datetime` SYMBOLS $get_month_range_inclusive;

    $start = Datetime('2021-01-02T02:05:06Z');
    $end = Datetime('2021-02-03T03:05:06Z');

    SELECT $get_month_range_inclusive($start, $end) as res
    '''

    response = run_yql_query(yql_client, query, files=[datetime_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['ListType', ['OptionalType', ['DataType', 'String']]]}
    assert result == [{'res': ['2021-01-01', '2021-02-01']}]
