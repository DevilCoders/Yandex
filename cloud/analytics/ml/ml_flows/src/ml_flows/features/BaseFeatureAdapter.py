from abc import abstractmethod
import typing as tp
from datetime import datetime, timedelta

import pyspark.sql.functions as F
from pyspark.sql.session import SparkSession
from pyspark.sql.dataframe import DataFrame as SparkDataFrame
from pyspark.sql.column import Column as SparkColumn

from clan_tools.data_adapters.YTAdapter import YTAdapter
from ml_flows.utils.spark import max_by, min_by, slope


KNOWN_AGGREGATIONS = [
    'MIN',  # minimum
    'MAX',  # maximum
    'AVG',  # average
    'STD',  # standard deviation
    'SUM',  # sum
    'SLP',  # slope
    'FST',  # first
    'LST',  # last
]
# ::TODO:: Добавить аггрегации описывающие timeseries


def take_table(tablename: str, start_table: str, end_table: str) -> bool:
    return start_table <= tablename <= end_table


class BaseFeatureAdapter:

    def __init__(self, spark: SparkSession, yt_adapter: YTAdapter,
                 base_spdf: SparkDataFrame, date_col: str = 'date',
                 functions_to_columns: tp.Optional[tp.Dict[str, tp.List[str]]] = None,
                 days_in_history: int = 30) -> None:
        self._spark = spark
        self._yt_adapter = yt_adapter
        self._days_in_history = days_in_history
        self._base_spdf = base_spdf.select('billing_account_id', date_col)
        self._functions_to_columns = functions_to_columns
        first_date = base_spdf.agg(F.min(date_col)).collect()[0][0]
        self._start_date = (datetime.strptime(first_date, '%Y-%m-%d') - timedelta(days=days_in_history)).strftime('%Y-%m-%d')
        self._end_date = base_spdf.agg(F.max(date_col)).collect()[0][0]
        self._date_col = date_col
        self._custom_date_colname = '_CUSTOM_TABLE_DATE_'

    def _generate_spark_cols(self) -> tp.List[SparkColumn]:
        if self._functions_to_columns is None:
            raise RuntimeError('Functions to columns config must be initialized')
        aggr_columns = []
        for func, columns in self._functions_to_columns.items():
            if func == 'MIN':
                aggr_columns.extend([F.min(clmn).alias(f'{clmn}_{self._days_in_history}_min') for clmn in columns])
            elif func == 'MAX':
                aggr_columns.extend([F.max(clmn).alias(f'{clmn}_{self._days_in_history}_max') for clmn in columns])
            elif func == 'AVG':
                aggr_columns.extend([F.mean(clmn).alias(f'{clmn}_{self._days_in_history}_avg') for clmn in columns])
            elif func == 'STD':
                aggr_columns.extend([F.stddev(clmn).alias(f'{clmn}_{self._days_in_history}_std') for clmn in columns])
            elif func == 'SUM':
                aggr_columns.extend([F.sum(clmn).alias(f'{clmn}_{self._days_in_history}_sum') for clmn in columns])
            elif func == 'SLP':
                aggr_columns.extend([slope(clmn, self._custom_date_colname).alias(f'{clmn}_{self._days_in_history}_slp') for clmn in columns])
            elif func == 'FST':
                aggr_columns.extend([min_by(clmn, self._custom_date_colname).alias(f'{clmn}_{self._days_in_history}_fst') for clmn in columns])
            elif func == 'LST':
                aggr_columns.extend([max_by(clmn, self._custom_date_colname).alias(f'{clmn}_{self._days_in_history}_lst') for clmn in columns])
            else:
                raise AttributeError(f'\nUnknown function: "{func}"')

        return aggr_columns

    @abstractmethod
    def get_double_features(self) -> SparkDataFrame:
        pass
