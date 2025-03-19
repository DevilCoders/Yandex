import typing

from yql.api.v1.client import YqlClient
from yql.client.operation import YqlOperationResultsRequest


def run_yql_select_query(yql_query: str) -> typing.Tuple[typing.List[typing.Any], str]:
    print('\n%s' % yql_query)
    client = YqlClient()
    query = client.query(yql_query, syntax_version=1)
    return _get_yql_query_results(query), query.operation_id


def get_yql_query_results_by_id(query_id: str) -> typing.List[typing.Any]:
    print('Loading YQL query %s result' % query_id)
    return _get_yql_query_results(YqlOperationResultsRequest(req_id=query_id))


# Returns YQL query results as list of tables,
# each table contains list of rows, row is a dict.
def _get_yql_query_results(query) -> typing.Union[typing.List[typing.Any], typing.List[typing.List[typing.Any]]]:
    def pre_start_callback(q):
        print(f'YQL query URL: {q.share_url}')

    query.run(pre_start_callback=pre_start_callback)

    tables = []
    for table in query:
        table.fetch_full_data()
        row_dicts = []
        for row in table.rows:
            row_dict = {}
            for cell, name, type in zip(row, table.column_names, table.column_print_types):
                row_dict[name] = cell
            row_dicts.append(row_dict)
        tables.append(row_dicts)

    return tables
