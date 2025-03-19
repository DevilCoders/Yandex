from decimal import Decimal

import pytest
from library.python import resource

from cloud.dwh.test_utils.yt.misc import parse_yql_query_result
from cloud.dwh.test_utils.yt.misc import parse_yql_query_schema
from cloud.dwh.test_utils.yt.misc import run_yql_query


@pytest.fixture(scope='session')
def numbers_sql():
    return 'numbers.sql', resource.find('yql/utils/numbers.sql')


def test_inf_constants(yql_client, yt_client, numbers_sql):
    query = '''
    PRAGMA Library('numbers.sql');
    IMPORT `numbers` SYMBOLS $DECIMAL_35_15_INF, $DECIMAL_35_15_NEG_INF;
    IMPORT `numbers` SYMBOLS $DECIMAL_35_9_INF, $DECIMAL_35_9_NEG_INF;

    SELECT
        $DECIMAL_35_15_INF AS res_35_15_inf,
        $DECIMAL_35_15_NEG_INF AS res_35_15_neg_inf,
        $DECIMAL_35_9_INF AS res_35_9_inf,
        $DECIMAL_35_9_NEG_INF AS res_35_9_neg_inf
    '''

    response = run_yql_query(yql_client, query, files=[numbers_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    expected_result = [{'res_35_15_inf': Decimal('Infinity'), 'res_35_15_neg_inf': Decimal('-Infinity'),
                        'res_35_9_inf': Decimal('Infinity'), 'res_35_9_neg_inf': Decimal('-Infinity')}]
    expected_schema = {'res_35_15_inf': ['OptionalType', ['DataType', 'Decimal', '35', '15']],
                       'res_35_15_neg_inf': ['OptionalType', ['DataType', 'Decimal', '35', '15']],
                       'res_35_9_inf': ['OptionalType', ['DataType', 'Decimal', '35', '9']],
                       'res_35_9_neg_inf': ['OptionalType', ['DataType', 'Decimal', '35', '9']]}

    assert result == expected_result
    assert schema == expected_schema


def test_eps_constants(yql_client, yt_client, numbers_sql):
    query = '''
    PRAGMA Library('numbers.sql');
    IMPORT `numbers` SYMBOLS $DECIMAL_35_15_EPS, $DECIMAL_35_9_EPS;

    SELECT
        $DECIMAL_35_15_EPS AS res_35_15_eps,
        $DECIMAL_35_9_EPS AS res_35_9_eps
    '''

    response = run_yql_query(yql_client, query, files=[numbers_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    expected_result = [{'res_35_15_eps': Decimal('1E-15'), 'res_35_9_eps': Decimal('1E-9')}]
    expected_schema = {'res_35_15_eps': ['DataType', 'Decimal', '35', '15'],
                       'res_35_9_eps': ['DataType', 'Decimal', '35', '9']}

    assert result == expected_result
    assert schema == expected_schema


def test_to_decimal_35_15(yql_client, yt_client, numbers_sql):
    query = '''
    PRAGMA Library('numbers.sql');
    IMPORT `numbers` SYMBOLS $to_decimal_35_15;

    SELECT $to_decimal_35_15('1.1') as res
    '''

    response = run_yql_query(yql_client, query, files=[numbers_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['OptionalType', ['DataType', 'Decimal', '35', '15']]}
    assert result == [{'res': Decimal('1.1')}]


def test_double_to_decimal_35_15(yql_client, yt_client, numbers_sql):
    query = '''
    PRAGMA Library('numbers.sql');
    IMPORT `numbers` SYMBOLS $double_to_decimal_35_15;

    SELECT $double_to_decimal_35_15(1.1) as res
    '''

    response = run_yql_query(yql_client, query, files=[numbers_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['OptionalType', ['DataType', 'Decimal', '35', '15']]}
    assert result == [{'res': Decimal('1.1')}]


def test_to_decimal_35_9(yql_client, yt_client, numbers_sql):
    query = '''
    PRAGMA Library('numbers.sql');
    IMPORT `numbers` SYMBOLS $to_decimal_35_9;

    SELECT $to_decimal_35_9('1.1') as res
    '''

    response = run_yql_query(yql_client, query, files=[numbers_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['OptionalType', ['DataType', 'Decimal', '35', '9']]}
    assert result == [{'res': Decimal('1.1')}]


def test_double_to_decimal_35_9(yql_client, yt_client, numbers_sql):
    query = '''
    PRAGMA Library('numbers.sql');
    IMPORT `numbers` SYMBOLS $double_to_decimal_35_9;

    SELECT $double_to_decimal_35_9(1.1) as res
    '''

    response = run_yql_query(yql_client, query, files=[numbers_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['OptionalType', ['DataType', 'Decimal', '35', '9']]}
    assert result == [{'res': Decimal('1.1')}]


def test_to_decimal_22_9(yql_client, yt_client, numbers_sql):
    query = '''
    PRAGMA Library('numbers.sql');
    IMPORT `numbers` SYMBOLS $to_decimal_22_9;

    SELECT $to_decimal_22_9('1.1') as res
    '''

    response = run_yql_query(yql_client, query, files=[numbers_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['OptionalType', ['DataType', 'Decimal', '22', '9']]}
    assert result == [{'res': Decimal('1.1')}]


def test_double_to_decimal_22_9(yql_client, yt_client, numbers_sql):
    query = '''
    PRAGMA Library('numbers.sql');
    IMPORT `numbers` SYMBOLS $double_to_decimal_22_9;

    SELECT $double_to_decimal_22_9(1.1) as res
    '''

    response = run_yql_query(yql_client, query, files=[numbers_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['OptionalType', ['DataType', 'Decimal', '22', '9']]}
    assert result == [{'res': Decimal('1.1')}]
