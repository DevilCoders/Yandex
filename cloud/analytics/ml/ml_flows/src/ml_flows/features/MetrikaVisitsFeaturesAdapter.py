import typing as tp
from datetime import datetime

import pyspark.sql.types as T
import pyspark.sql.functions as F
from pyspark.sql.functions import col
from pyspark.sql.session import SparkSession
from pyspark.sql.dataframe import DataFrame as SparkDataFrame

from clan_tools.data_adapters.YTAdapter import YTAdapter
from ml_flows.features.BaseFeatureAdapter import KNOWN_AGGREGATIONS, BaseFeatureAdapter, take_table


class MetrikaVisitsFeaturesAdapter(BaseFeatureAdapter):

    METRIKA_VISITS_FOLDER = '//home/cloud_analytics/ml/ml_model_features/by_puid/metrika/visits'
    BAID_PUID_DICT = '//home/cloud_analytics/ml/ml_model_features/dict/baid_puid'

    def __init__(self, spark: SparkSession, yt_adapter: YTAdapter,
                 base_spdf: SparkDataFrame, date_col: str = 'date',
                 functions_to_columns: tp.Optional[tp.Dict[str, tp.List[str]]] = None,
                 days_in_history: int = 30) -> None:
        super().__init__(spark, yt_adapter, base_spdf, date_col, functions_to_columns, days_in_history)

        per_s = datetime.strptime(self._start_date, '%Y-%m-%d').strftime('%Y-%m')
        per_e = datetime.strptime(self._end_date, '%Y-%m-%d').strftime('%Y-%m')

        self._tables = [f'{self.METRIKA_VISITS_FOLDER}/{tbl}' for tbl in self._yt_adapter.yt.list(self.METRIKA_VISITS_FOLDER) if take_table(tbl, per_s, per_e)]

    def get_double_features(self) -> SparkDataFrame:
        spdf_mtrv = self._spark.read.option('mergeschema', 'true').yt(*self._tables).withColumn(self._custom_date_colname, col('date')).drop('date')

        spdf_mtrv_features = [clmn[0] for clmn in spdf_mtrv.dtypes if (clmn[1] in ['int', 'smallint', 'double', 'uint64', 'bigint'])]
        spdf_mtrv_features.remove('puid')
        spdf_mtrv = spdf_mtrv.select(
            'puid', self._custom_date_colname,
            *[col(clnm).astype(T.LongType()).astype(T.DoubleType()).alias(clnm) for clnm in spdf_mtrv_features])

        # DataFrame default columns
        if self._functions_to_columns is None:
            spdf_mtrv = spdf_mtrv.select(*[col(colname).alias(colname.replace(' ', '_')) for colname in spdf_mtrv.columns])

            spdf_mtrv_double_features = [clmn[0] for clmn in spdf_mtrv.dtypes if clmn[1] == 'double']
            self._functions_to_columns = {fn: spdf_mtrv_double_features for fn in KNOWN_AGGREGATIONS}

        # final dataset
        spdf_dict = self._spark.read.yt(self.BAID_PUID_DICT)
        spdf_features = (
            self._base_spdf
            .join(spdf_dict, on='billing_account_id')
            .join(spdf_mtrv, on=(
                (spdf_dict.puid == spdf_mtrv.puid) &
                (F.date_add(self._base_spdf[self._date_col], -self._days_in_history) <= spdf_mtrv[self._custom_date_colname]) &
                (self._base_spdf[self._date_col] > spdf_mtrv[self._custom_date_colname])
            ), how='left')
            .groupby('billing_account_id', 'date')
            .agg(*self._generate_spark_cols())
        )

        return spdf_features

    def get_category_features(self) -> SparkDataFrame:
        spdf_mtrh = (
            self._spark.read.option('mergeschema', 'true').yt(*self._tables)
            .select(
                'puid',
                'date',
                'domain_zone'
            )
        )

        spdf_dict = self._spark.read.yt(self.BAID_PUID_DICT)
        spdf_features = (
            self._base_spdf
            .join(spdf_dict, on='billing_account_id')
            .join(spdf_mtrh, on=(
                (spdf_dict.puid == spdf_mtrh.puid) &
                (F.date_add(self._base_spdf[self._date_col], -self._days_in_history) <= spdf_mtrh[self._custom_date_colname]) &
                (self._base_spdf[self._date_col] > spdf_mtrh[self._custom_date_colname])
            ), how='left')
            .drop(spdf_mtrh.puid, spdf_mtrh.date)
        )
        return spdf_features
