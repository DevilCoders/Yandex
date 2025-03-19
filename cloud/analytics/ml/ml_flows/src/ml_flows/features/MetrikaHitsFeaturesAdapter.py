import typing as tp
from datetime import datetime

import pyspark.sql.types as T
import pyspark.sql.functions as F
from pyspark.sql.functions import col
from pyspark.sql.session import SparkSession
from pyspark.sql.dataframe import DataFrame as SparkDataFrame

from clan_tools.data_adapters.YTAdapter import YTAdapter
from ml_flows.features.BaseFeatureAdapter import KNOWN_AGGREGATIONS, BaseFeatureAdapter, take_table


class MetrikaHitsFeaturesAdapter(BaseFeatureAdapter):

    METRIKA_HITS_FOLDER = '//home/cloud_analytics/ml/ml_model_features/by_puid/metrika/hits'
    BAID_PUID_DICT = '//home/cloud_analytics/ml/ml_model_features/dict/baid_puid'

    def __init__(self, spark: SparkSession, yt_adapter: YTAdapter,
                 base_spdf: SparkDataFrame, date_col: str = 'date',
                 functions_to_columns: tp.Optional[tp.Dict[str, tp.List[str]]] = None,
                 days_in_history: int = 30) -> None:
        super().__init__(spark, yt_adapter, base_spdf, date_col, functions_to_columns, days_in_history)

        per_s = datetime.strptime(self._start_date, '%Y-%m-%d').strftime('%Y-%m')
        per_e = datetime.strptime(self._end_date, '%Y-%m-%d').strftime('%Y-%m')

        self._tables = [f'{self.METRIKA_HITS_FOLDER}/{tbl}' for tbl in self._yt_adapter.yt.list(self.METRIKA_HITS_FOLDER) if take_table(tbl, per_s, per_e)]

    def get_double_features(self) -> SparkDataFrame:
        spdf__mtrh = self._spark.read.option('mergeschema', 'true').yt(*self._tables).withColumn(self._custom_date_colname, col('date')).drop('date')

        spdf__mtrh_features = [clmn[0] for clmn in spdf__mtrh.dtypes if (clmn[1] in ['int', 'smallint', 'double', 'uint64', 'bigint'])]
        spdf__mtrh_features.remove('puid')
        spdf__mtrh = spdf__mtrh.select(
            'puid', self._custom_date_colname,
            *[col(clnm).astype(T.LongType()).astype(T.DoubleType()).alias(clnm) for clnm in spdf__mtrh_features])

        # DataFrame default columns
        if self._functions_to_columns is None:
            spdf__mtrh = spdf__mtrh.select(*[col(colname).alias(colname.replace(' ', '_')) for colname in spdf__mtrh.columns])

            spdf__mtrh_double_features = [clmn[0] for clmn in spdf__mtrh.dtypes if clmn[1] == 'double']
            self._functions_to_columns = {fn: spdf__mtrh_double_features for fn in KNOWN_AGGREGATIONS}

        # final dataset
        spdf_dict = self._spark.read.yt(self.BAID_PUID_DICT)
        spdf_features = (
            self._base_spdf
            .join(spdf_dict, on='billing_account_id')
            .join(spdf__mtrh, on=(
                (spdf_dict.puid == spdf__mtrh.puid) &
                (F.date_add(self._base_spdf[self._date_col], -self._days_in_history) <= spdf__mtrh[self._custom_date_colname]) &
                (self._base_spdf[self._date_col] > spdf__mtrh[self._custom_date_colname])
            ), how='left')
            .groupby('billing_account_id', 'date')
            .agg(*self._generate_spark_cols())
        )

        return spdf_features
