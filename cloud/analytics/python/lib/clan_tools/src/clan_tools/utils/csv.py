import typing as tp
import pandas as pd
from io import BytesIO


def str_to_df(data_str: str, **kwargs: tp.Dict[str, tp.Any]) -> pd.DataFrame:
    return pd.read_csv(BytesIO(data_str), na_values='\\N', **kwargs)  # type: ignore


def read_ch_csv(path: str) -> pd.DataFrame:
    return pd.read_csv(path, na_values='\\N')

__all__ = ['str_to_df', 'read_ch_csv']
