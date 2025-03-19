import typing as tp
from datetime import datetime

import pyspark.sql.types as T
import pyspark.sql.functions as F
from pyspark.sql.functions import col
from pyspark.sql.session import SparkSession
from pyspark.sql.dataframe import DataFrame as SparkDataFrame

from clan_tools.data_adapters.YTAdapter import YTAdapter
from ml_flows.features.BaseFeatureAdapter import KNOWN_AGGREGATIONS, BaseFeatureAdapter, take_table


class CryptaFeaturesAdapter(BaseFeatureAdapter):

    CRYPTA_FOLDER = '//home/cloud_analytics/ml/ml_model_features/by_puid/crypta'
    BAID_PUID_DICT = '//home/cloud_analytics/ml/ml_model_features/dict/baid_puid'

    def __init__(self, spark: SparkSession, yt_adapter: YTAdapter,
                 base_spdf: SparkDataFrame, date_col: str = 'date',
                 functions_to_columns: tp.Optional[tp.Dict[str, tp.List[str]]] = None,
                 days_in_history: int = 30) -> None:
        super().__init__(spark, yt_adapter, base_spdf, date_col, functions_to_columns, days_in_history)

        per_s = datetime.strptime(self._start_date, '%Y-%m-%d').strftime('%Y-%m')
        per_e = datetime.strptime(self._end_date, '%Y-%m-%d').strftime('%Y-%m')

        self._tables = [f'{self.CRYPTA_FOLDER}/{tbl}' for tbl in self._yt_adapter.yt.list(self.CRYPTA_FOLDER) if take_table(tbl, per_s, per_e)]

    def get_double_features(self) -> SparkDataFrame:
        spdf_crpt = self._spark.read.yt(*self._tables).withColumn(self._custom_date_colname, col('date')).drop('date')

        spdf_crpt_boolean_features = [clmn[0] for clmn in spdf_crpt.dtypes if clmn[1] == 'boolean']

        transformed_columns = []
        for clmn in spdf_crpt.columns:
            if clmn in spdf_crpt_boolean_features:
                transformed_columns.append(col(clmn).astype(T.DoubleType()).alias(clmn))
            else:
                transformed_columns.append(col(clmn))
        spdf_crpt = spdf_crpt.select(transformed_columns)

        # DataFrame default columns
        if self._functions_to_columns is None:
            spdf_crpt = spdf_crpt.select(*[col(colname).alias(colname.replace(' ', '_')) for colname in spdf_crpt.columns])

            spdf_crpt_double_features = [clmn[0] for clmn in spdf_crpt.dtypes if clmn[1] == 'double']
            self._functions_to_columns = {fn: spdf_crpt_double_features for fn in KNOWN_AGGREGATIONS}

        # final dataset
        spdf_dict = self._spark.read.yt(self.BAID_PUID_DICT)
        spdf_features = (
            self._base_spdf
            .join(spdf_dict, on='billing_account_id')
            .join(spdf_crpt, on=(
                (spdf_dict.puid == spdf_crpt.puid) &
                (F.date_add(self._base_spdf[self._date_col], -self._days_in_history) <= spdf_crpt[self._custom_date_colname]) &
                (self._base_spdf[self._date_col] > spdf_crpt[self._custom_date_colname])
            ), how='left')
            .groupby('billing_account_id', 'date')
            .agg(*self._generate_spark_cols())
        )

        return spdf_features
