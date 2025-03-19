import typing as tp
import os
import json
import logging
import pandas as pd
from textwrap import dedent
from yql.api.v1.client import YqlClient
from yql.client.operation import YqlSqlOperationRequest, YqlOperationResultsRequest

from clan_tools.utils.timing import timing
from clan_tools.data_adapters.ExecuteQueryAdapter import ExecuteQueryAdapter

logger = logging.getLogger(__name__)


class YQLAdapter(ExecuteQueryAdapter):

    def __init__(self, token: tp.Optional[str] = None, cluster: str = 'hahn') -> None:
        if token is None:
            token = os.environ['YQL_TOKEN']
        self._client = YqlClient(token=token, table_format='csv', db=cluster)

    @timing
    def execute_query(self, query: str = '', *args: tp.Tuple[tp.Any], **kwargs: tp.Dict[str, tp.Any]) -> None:
        raise AttributeError(dedent("""
            Bound method 'YQLAdapter.execute_query' is DEPRECATED
            Use either 'YQLAdapter.run_query' or 'YQLAdapter.run_query_to_pandas'
        """))

    def run_query(self, query: str, dirname: tp.Optional[str] = None,
                  file_or_folder_or_none: tp.Optional[str] = None) -> YqlOperationResultsRequest:
        logger.debug(f"Executing YQL query:\n {query}")
        query_req: YqlSqlOperationRequest = self._client.query(query, syntax_version=1)
        if dirname is not None:
            if file_or_folder_or_none is None:
                file_or_folder_or_none = ''
            self.attach_files(dirname, file_or_folder_or_none, query_req)
        query_req.run()
        query_res = query_req.get_results()
        logger.debug(query_res)
        self.is_success(query_req)
        return query_res

    def run_query_to_pandas(self, query: str, dirname: tp.Optional[str] = None,
                            file_or_folder_or_none: tp.Optional[str] = None) -> pd.DataFrame:
        query_res = self.run_query(query, dirname, file_or_folder_or_none)
        return query_res.full_dataframe

    @staticmethod
    def attach_files(dirname: str, file_or_folder_or_none: str, req: YqlSqlOperationRequest) -> YqlSqlOperationRequest:
        path = os.path.join(os.path.dirname(os.path.realpath(dirname)), file_or_folder_or_none)
        if os.path.isfile(path):
            logger.debug(f'Attaching file {path}')
            req.attach_file(path, alias=file_or_folder_or_none)
        else:
            for file in os.listdir(path):
                if file.endswith('.py') or file.endswith('.sql'):
                    abs_path = os.path.join(path, file)
                    logger.debug(f'Attaching file {abs_path}')
                    req.attach_file(abs_path, alias=file)
        return req

    @staticmethod
    def is_success(request: YqlSqlOperationRequest) -> bool:
        status = True
        if request.is_success:
            logger.debug('Request is successful!')
        else:
            logger.error(f'Request {request.share_url} is {request.status}')
            if request.errors:
                logger.error(f'Returned errors: {request.errors}')
            status = False
            raise RuntimeError('Request failed!')
        return status

    @staticmethod
    def yql_respone_request_to_json(req: YqlSqlOperationRequest, result_table_path: str) -> None:
        req.run()
        req.get_results()

        with open('output.json', 'w') as f:
            json.dump({
                "operation_id": req.share_url,
                "table_path": result_table_path
                }, f)
