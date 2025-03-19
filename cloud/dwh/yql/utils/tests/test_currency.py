from decimal import Decimal

import pytest
from library.python import resource

from cloud.dwh.test_utils.yt.misc import parse_yql_query_result
from cloud.dwh.test_utils.yt.misc import parse_yql_query_schema
from cloud.dwh.test_utils.yt.misc import run_yql_query


@pytest.fixture(scope='session')
def currency_sql():
    return 'currency.sql', resource.find('yql/utils/currency.sql')


@pytest.mark.parametrize('constant, expected', [
    ('$RUB', 'RUB'),
    ('$USD', 'USD'),
    ('$KZT', 'KZT'),
])
def test_currency_constants(yql_client, currency_sql, constant, expected):
    query = f'''
    PRAGMA Library('currency.sql');
    IMPORT `currency` SYMBOLS {constant};

    SELECT {constant} as res
    '''

    response = run_yql_query(yql_client, query, files=[currency_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['DataType', 'String']}
    assert result == [{'res': expected}]


@pytest.mark.parametrize('amount, date, currency, expected', [
    ('2.36', '2018-12-31', 'RUB', '2'),
    ('2.4', '2019-01-01', 'RUB', '2'),
    ('2.24', '2018-12-31', 'KZT', '2'),
    ('2.24', '2019-01-01', 'KZT', '2'),
    ('1.2', '2018-12-31', 'USD', '1.2'),
    ('1.2', '2019-01-01', 'USD', '1.2'),
])
def test_get_vat_decimal_35_15(yql_client, currency_sql, amount, date, currency, expected):
    query = f'''
    PRAGMA Library('currency.sql');
    IMPORT `currency` SYMBOLS $get_vat_decimal_35_15;
    SELECT $get_vat_decimal_35_15(Decimal('{amount}', 35, 15), '{date}', '{currency}') as res
    '''

    response = run_yql_query(yql_client, query, files=[currency_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['DataType', 'Decimal', '35', '15']}
    assert result == [{'res': Decimal(expected)}]
