import pytest
from library.python import resource

from cloud.dwh.test_utils.yt.misc import parse_yql_query_result
from cloud.dwh.test_utils.yt.misc import parse_yql_query_schema
from cloud.dwh.test_utils.yt.misc import run_yql_query


@pytest.fixture(scope='session')
def helpers_sql():
    return 'helpers.sql', resource.find('yql/utils/helpers.sql')


def test_get_md5(yql_client, helpers_sql):
    query = '''
    PRAGMA Library('helpers.sql');
    IMPORT `helpers` SYMBOLS $get_md5;

    SELECT $get_md5((1,2,3,[3,4])) as res
    '''

    response = run_yql_query(yql_client, query, files=[helpers_sql])

    result = parse_yql_query_result(response)
    schema = parse_yql_query_schema(response)

    assert schema == {'res': ['DataType', 'String']}
    assert result == [{'res': '23a9822af20f29eec7fa4b62270819c0'}]
