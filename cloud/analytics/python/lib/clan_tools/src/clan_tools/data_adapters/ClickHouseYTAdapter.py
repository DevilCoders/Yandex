import os
import logging
import typing as tp
from requests.exceptions import HTTPError  # type: ignore
from textwrap import dedent

from clan_tools.data_adapters.ClickHouseRestAdapter import ClickHouseRestAdapter

logger = logging.getLogger(__name__)


class ClickHouseYTAdapter(ClickHouseRestAdapter):

    def __init__(self, token: tp.Optional[str] = None, cluster: str = 'hahn', alias: str = "*cloud_analytics",  timeout: int = 600) -> None:
        """
        :param token: user YT token
        :param alias: ClickHouse click, defaults to "*cloud_analytics". For public use None
        """
        pool = alias or "*ch_public"
        proxy = "http://{}.yt.yandex.net".format(cluster)
        token = token or os.environ['YT_TOKEN']
        self._post_conf = dict(url=f"{proxy}/query?database={pool}&password={token}", timeout=timeout)

    def insert_into(self, result_table_path: str, select_query: str, append: bool = True, create_if_not_exists: bool = True) -> None:
        if create_if_not_exists:
            self.execute_query(dedent(f'''
                CREATE TABLE "{result_table_path}" ENGINE = YtTable() AS
                    {select_query}
            '''), to_pandas=False)

        else:
            try:
                append_str = 'true' if append else 'false'
                self.execute_query(dedent(f'''
                    INSERT INTO "<append=%{append_str}>{result_table_path}"
                        {select_query}
                '''), to_pandas=False)

            except HTTPError as exc:
                logger.exception(exc)
                raise exc
