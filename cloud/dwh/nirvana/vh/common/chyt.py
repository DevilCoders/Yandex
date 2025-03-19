import json
from dataclasses import dataclass
import requests
import typing as tp
import pandas as pd
import numpy as np
import logging


@dataclass
class ClientConfig:
    cluster: str
    clique: str
    yt_token: str
    retry_count: int = 5


def _update_types(df_raw: pd.DataFrame) -> pd.DataFrame:
    """
    Update all string types in table to list, int, float, str.
    if 'id' in column name and column type may
    be like int - does not update this column.
    :param df_raw: raw dataframe which column types need to be updated
    :return: pd.DataFrame (updated table)
    """
    def _raise():
        raise Exception

    df = df_raw.copy()
    for column in df.columns:
        # list updater
        try:
            df[column] = df[column].apply(
                lambda x: json.loads(x) if isinstance(json.loads(x), list) else _raise()
            )
            continue
        except Exception:
            pass
        try:
            df[column] = df[column].apply(
                lambda x: json.loads(x.replace("'", '"')) if isinstance(json.loads(x.replace("'", '"')), list) else _raise()
            )
            continue
        except Exception:
            pass
        # id checker
        try:
            if ("id" in column and "paid" not in column) and not pd.isnull(df[column].astype(int).sum()):
                continue
        except Exception:
            pass
        # int updater
        try:
            df[column] = df[column].astype(int)
            continue
        except Exception:
            pass

        # float updater
        try:
            df[column] = df[column].astype(float)
            continue
        except Exception:
            pass
    return df


class ChytClient:
    def __init__(self, config: ClientConfig):
        self._config = config

    def _raw_execute_yt_query(self, query: str, timeout=600) -> tp.List[str]:
        proxy = f"http://{self._config.cluster}.yt.yandex.net"
        s = requests.Session()
        url = f"{proxy}/query?database={self._config.clique}&password={self._config.yt_token}" \
              "&enable_optimize_predicate_expression=0"
        resp = s.post(url, data=query, timeout=timeout)

        if not resp.ok:
            print(query)
            print(resp.text)

        resp.raise_for_status()
        rows = resp.text.strip().split('\n')
        return rows

    def _raw_chyt_execute_query(self, query: str,
                                columns: tp.Optional[str] = None) -> pd.DataFrame:
        counter = 0
        while True:
            try:
                result = self._raw_execute_yt_query(query=query)
                if columns is None:
                    df_raw = pd.DataFrame([row.split('\t')
                                           for row in result[1:]],
                                          columns=result[0].split('\t'))
                else:
                    df_raw = pd.DataFrame([row.split('\t')
                                           for row in result], columns=columns)
                return df_raw
            except Exception as err:
                logging.info(f"Caught exception: {err}\nRetrying...")
                counter += 1
                if counter > self._config.retry_count:
                    raise err

    def execute_query(self, query: str,
                      columns: tp.Optional[str] = None):
        df = self._raw_chyt_execute_query(query, columns)
        df = df.replace('\\N', np.NaN)
        return _update_types(df)
