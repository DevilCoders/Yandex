import typing as tp
import orjson
import logging
import requests  # type: ignore
import pandas as pd

from clan_tools.utils.timing import timing
from clan_tools.data_adapters.ExecuteQueryAdapter import ExecuteQueryAdapter

logger = logging.getLogger(__name__)


class ClickHouseRestAdapter(ExecuteQueryAdapter):
    def __init__(self, post_conf: tp.Dict[str, tp.Any]) -> None:
        self._post_conf = post_conf

    @timing
    def execute_query(self, query: str, compression: tp.Optional[str] = 'gzip', to_pandas: bool = False) -> tp.Union[str, pd.DataFrame]:
        '''Query to ClickHouse over YT. Uses native ClickHouse Rest API'''
        res = None
        if to_pandas:
            query += '\n FORMAT JSONCompact'
        logger.debug(f"Executing query: {query}")

        session = requests.Session()
        if to_pandas and (compression is not None):
            session.headers.update({'Accept-Encoding': compression})

        resp = session.post(data=query, **self._post_conf)
        if resp.status_code != 200:
            logger.error("Response status: %s", resp.status_code)
            logger.error("Response headers: %s", resp.headers)
            logger.error("Response content: %s", resp.text)
        resp.raise_for_status()
        logger.debug(f"Time spent for response: {resp.elapsed.total_seconds()} seconds")

        if to_pandas:
            json_data = orjson.loads(resp.content)
            res = pd.DataFrame.from_records(json_data['data'], columns=map(lambda x: x['name'], json_data['meta'])).infer_objects()
        else:
            res = resp.text
        return res
