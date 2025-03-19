import hashlib
from typing import Dict
from typing import List
from typing import Optional
from typing import TYPE_CHECKING
from typing import Tuple

import yatest.common
import yt.wrapper as yt

if TYPE_CHECKING:
    from yql.api.v1.client import YqlClient
    from yql.client.operation import YqlOperationResultsRequest


def get_test_prefix():
    return f'yt_tmp_{hashlib.md5(yatest.common.context.test_name.encode()).hexdigest()}'


def create_map_node(yt_client: yt.YtClient, path: str, test_prefix: str = None):
    path = f'{test_prefix or get_test_prefix()}/{path}'
    yt_client.create('map_node', path, recursive=True, ignore_existing=True)


def create_table(yt_client: yt.YtClient, path: str, schema: Dict[str, str], data: List[dict], test_prefix: str = None, extra_attrs: dict = None):
    schema = [{'name': field_name, 'type': field_type} for field_name, field_type in schema.items()]
    path = f'{test_prefix or get_test_prefix()}/{path}'
    attributes = {'schema': schema}

    if extra_attrs:
        attributes.update(attributes)

    yt_client.create('table', path, recursive=True, ignore_existing=False, attributes=attributes)
    yt_client.write_table(
        table=path,
        input_stream=data,
        format=yt.JsonFormat()
    )


def run_yql_query(yql_client: 'YqlClient', query: str, files: Optional[List[Tuple[str, str]]] = None) -> 'YqlOperationResultsRequest':
    request = yql_client.query(query)

    for name, data in files or []:
        request.attach_file_data(data=data, name=name)

    request.run()
    result = request.get_results()

    assert result.is_success, [str(error) for error in result.errors]

    return result


def parse_yql_query_result(result: 'YqlOperationResultsRequest') -> List[dict]:
    result = list(result)
    assert len(result) == 1, result

    result = result[0]
    data = []

    for row in result.rows:
        data.append({k: v for (k, _), v in zip(result.columns, row)})

    return data


def parse_yql_query_schema(result: 'YqlOperationResultsRequest') -> dict:
    result = list(result)
    assert len(result) == 1, result

    return {n: t for n, t in result[0].columns}
