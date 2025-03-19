import os
import time
import logging
import numpy as np
import pandas as pd
from datetime import datetime
from typing import List, Dict, Union, Optional, Any, Iterable
import yt.wrapper as yt
from clan_tools.utils.timing import timing
from clan_tools.utils.time import time_to_unix

logger = logging.getLogger(__name__)


class YTAdapter:
    def __init__(self, token: Optional[str] = None, cluster: str = 'hahn', pool: str = 'cloud_analytics_pool') -> None:
        """Adapter which works directly with YT. More docs here https://yt.yandex-team.ru/docs/
        :param token: You can get it here https://oauth.yt.yandex.net/
        :type token: str
        :param cluster: In most cases you don't need to chage that, defaults to 'hahn'
        :type cluster: str, optional
        :param pool: use None to use public pool, defaults to 'cloud_analytics_pool'
        :type pool: str, optional
        """
        yt.config["proxy"]["url"] = cluster
        yt.config["token"] = token
        if pool is not None:
            yt.config["pool"] = pool

        if token is None:
            token = os.environ['YT_TOKEN']
        self.yt = yt

    @timing
    def save_result(self, result_path: str, schema: Union[List[Dict[str, str]], Dict[str, str], None],
                    df: pd.DataFrame, append: bool = True, default_type: str = 'string') -> None:
        """Save pandas dataframe with known type schema to YT result path

        :param result_path: Path in YT, starts rith '//'
        :type result_path: str
        :type default_type: str
        :param default_type: string or any
        :param schema: Columns path schema
        >>>   yt_schema = [
        >>>        {"name": "billing_account_id", "type": "string"},
        >>>        {"name": "create_time", "type": "datetime"},
        >>>        {"name": "is_target_pred", "type": "boolean"},
        >>>        {"name": "conf_is_target", "type": "double"},
        >>>        {"name": "conf_is_not_target", "type": "double"}
        >>>    ]
            Also you can use dict_to_schema method to convert dict to schema.
            Also you can use just dict.
        >>>   yt_schema = {"col": "string", 'keys':'list:string'}

        :type schema: Union[List[Dict[str, str]], Dict]
        :param df: Dataframe to store
        :type df: pd.DataFrame
        :param append: Will append rows to to table if enabled, defaults to True
        :type append: bool, optional
        """
        logger.info(f'Saving data to {result_path}')
        table = yt.TablePath(result_path, append=append)
        yt_schema = None
        if (schema is None):
            yt_schema = self.apply_type(schema, df, default_type)
        elif isinstance(schema, dict):
            yt_schema = self.dict_to_schema(schema)
        else:
            yt_schema = schema
        if not yt.exists(table):
            yt.create(type='table', path=table, attributes={"schema": yt_schema})
        yt.write_table(table, df.replace([np.nan], [None]).to_dict(orient='records'), format=yt.JsonFormat(attributes={"encode_utf8": False}))
        logger.info("Results are saved")

    def read_table(self, path: str, to_pandas: bool = True) -> Union[pd.DataFrame, Iterable[Any]]:
        if to_pandas:
            return pd.DataFrame(self.yt.read_table(path))
        else:
            return self.yt.read_table(path)

    @staticmethod
    def get_pandas_default_schema(df: pd.DataFrame) -> List[Dict[str, str]]:
        yt_schema = []
        for col in df.columns:
            list_of_types = [type(elem) for elem in df[col] if elem is not None]
            curr_type = list_of_types[0]
            if all([single_type == curr_type for single_type in list_of_types]):
                if curr_type is str:
                    yt_schema.append({"name": col, "type": "string"})
                elif curr_type is float:
                    yt_schema.append({"name": col, "type": "double"})
                elif curr_type is int:
                    yt_schema.append({"name": col, "type": "int64"})
                elif curr_type is bool:
                    yt_schema.append({"name": col, "type": "boolean"})
                else:
                    yt_schema.append({"name": col, "type": "any"})
            else:
                yt_schema.append({"name": col, "type": "string"})
        return yt_schema

    @staticmethod
    def datetime2ytdatetime(dt: datetime) -> int:
        return int(dt.timestamp())

    @staticmethod
    def dict_to_schema(column_type_dict: Dict[str, str]) -> List[Dict[str, str]]:
        return [{"name": col_name, "type": col_type} for col_name, col_type in column_type_dict.items()]

    @staticmethod
    def apply_type(raw_schema: Optional[Dict[str, Any]], df: pd.DataFrame, default_type: str = 'string') -> List[Dict[str, str]]:
        if raw_schema is not None:
            for key in raw_schema:
                if 'list:' not in str(raw_schema[key]) and raw_schema[key] != 'datetime':
                    df[key] = df[key].astype(raw_schema[key])
                if raw_schema[key] == 'datetime':
                    df[key] = df[key].apply(lambda x: time_to_unix(x))

        schema = []
        for col in df.columns:
            if raw_schema is not None and raw_schema.get(col) is not None and raw_schema[col] == 'datetime':
                schema.append({"name": col, 'type': 'datetime', 'required': False})
                continue
            if df[col].dtype == int:
                schema.append({"name": col, 'type': 'int64', 'required': False})
            elif df[col].dtype == str:
                schema.append({"name": col, 'type': 'string', 'required': False})
            elif df[col].dtype == bool:
                schema.append({"name": col, 'type': 'boolean', 'required': False})
            elif (df[col].dtype == float) or (df[col].dtype == np.float32):
                schema.append({"name": col, 'type': 'double', 'required': False})
            elif raw_schema is not None and raw_schema.get(col) is not None and 'list:' in str(raw_schema[col]):
                second_type = raw_schema[col].split("list:")[-1]
                schema.append({"name": col, 'type_v3': {"type_name": 'list', "item": {"type_name": "optional", "item": second_type}}})
            else:
                schema.append({"name": col, 'type': default_type, 'required': False})
        logger.debug(schema)
        return schema

    def last_table_name(self, path_to_dir: str) -> str:
        tables = yt.list(path_to_dir, attributes=['type'])
        tables_names = []

        for table in tables:
            if table.attributes['type'] == 'table':
                table_name = str(table)
                tables_names.append(table_name)
        min_name = np.nanargmax(tables_names)
        return f'{path_to_dir}/{str(tables_names[min_name])}'

    def create_paths(self, paths_iter: List[str]) -> None:
        for path in paths_iter:
            if not self.yt.exists(path):
                self.yt.mkdir(path, recursive=True)

    def optimize_chunk_number(self, path: str, optimize_for: Optional[str] = None, chunk_size_in_bytes: int = 200*2**20, retries_num: int = 10) -> None:
        """Transforms source table to destination table writing data with desired chunk size and table format.
        More information https://pydoc.yt.yandex.net/yt.wrapper.html#yt.wrapper.client_impl.YtClient.transform

        :param path: path to table in YT
        :type path: str
        :param optimize_for: table format, possible values "scan", "lookup", defaults to None (which make current table format)
        :type optimize_for: str, optional
        :param chunk_size_in_bytes: desired chunk size in bytes, defaults to 200*2**20 (which equals to 200MB)
        :type chunk_size_in_bytes: int
        """
        logger.info(f'Optimizing table in path {path}...')
        for i in range(1, retries_num+1):
            logger.info(f'Attempt {i}')
            try:
                self.yt.transform(path, desired_chunk_size=chunk_size_in_bytes, optimize_for=optimize_for)
                break
            except Exception:
                time.sleep(min(60, 1.5**i))

    def leave_last_N_tables(self, folder: str, N: int) -> None:
        """Removes tables in folder except of last N (by alphabetical order)

        :param folder: path to folder in YT
        :type folder: str
        :param N: number of tables to leave in folder
        :type N: int
        """
        tables_in_folder = self.yt.list(folder, attributes=['type'])
        sorted_table_pathes = sorted([f'{folder}/{str(table)}' for table in tables_in_folder if (table.attributes['type'] == 'table')])

        for table in sorted_table_pathes[:-N]:
            logger.info(f'Delete table {table}')
            self.yt.remove(table)

__all__ = ['YTAdapter']
