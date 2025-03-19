import typing as tp
import logging
from textwrap import dedent

from clan_tools.data_adapters.ClickHouseAdapter import ClickHouseAdapter

from tqdm import tqdm

logger = logging.getLogger()


def drop_temp_tables(table_name: str, ch_adapter: tp.Optional[ClickHouseAdapter] = None) -> tp.List[str]:
    if ch_adapter is None:
        ch_adapter = ClickHouseAdapter()
    tables_to_drop = ch_adapter.execute_query(dedent(f"""
        SELECT DISTINCT
            table
        FROM system.parts
        WHERE active
        AND table LIKE '{table_name}%'
        AND table != '{table_name}'
    """), to_pandas=True)

    tables_list = tables_to_drop.values.tolist()  # type: ignore
    for t in tqdm(tables_list):
        ch_adapter.execute_query(f'DROP TABLE cloud_analytics.{t[0]} ON CLUSTER cloud_analytics')
    return tables_list

__all__ = ['drop_temp_tables']
