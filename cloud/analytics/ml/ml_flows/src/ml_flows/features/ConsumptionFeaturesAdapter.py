import typing as tp
from datetime import datetime

import pyspark.sql.functions as F
from pyspark.sql.functions import col
from pyspark.sql.session import SparkSession
from pyspark.sql.dataframe import DataFrame as SparkDataFrame

from clan_tools.data_adapters.YTAdapter import YTAdapter
from ml_flows.features.BaseFeatureAdapter import KNOWN_AGGREGATIONS, BaseFeatureAdapter, take_table


class ConsumptionFeaturesAdapter(BaseFeatureAdapter):

    CONSUMPTION_FOLDER = '//home/cloud_analytics/ml/ml_model_features/by_baid/consumption'

    def __init__(self, spark: SparkSession, yt_adapter: YTAdapter,
                 base_spdf: SparkDataFrame, date_col: str = 'date',
                 functions_to_columns: tp.Optional[tp.Dict[str, tp.List[str]]] = None,
                 days_in_history: int = 30) -> None:
        super().__init__(spark, yt_adapter, base_spdf, date_col, functions_to_columns, days_in_history)

        per_s = datetime.strptime(self._start_date, '%Y-%m-%d').strftime('%Y-%m')
        per_e = datetime.strptime(self._end_date, '%Y-%m-%d').strftime('%Y-%m')

        self._tables = [f'{self.CONSUMPTION_FOLDER}/{tbl}' for tbl in self._yt_adapter.yt.list(self.CONSUMPTION_FOLDER) if take_table(tbl, per_s, per_e)]

    def get_double_features(self) -> SparkDataFrame:
        spdf_cons = self._spark.read.yt(*self._tables).withColumn(self._custom_date_colname, col('date')).drop('date')

        # DataFrame default columns
        if self._functions_to_columns is None:
            spdf_cons = spdf_cons.select(*[col(colname).alias(colname.replace(' ', '_')) for colname in spdf_cons.columns])
            spdf_cons_double_features = [clmn[0] for clmn in spdf_cons.dtypes if clmn[1] == 'double']
            self._functions_to_columns = {fn: spdf_cons_double_features for fn in KNOWN_AGGREGATIONS}

        # final dataset
        spdf_features = (
            self._base_spdf
            .join(spdf_cons, on=(
                (self._base_spdf.billing_account_id == spdf_cons.billing_account_id) &
                (F.date_add(self._base_spdf[self._date_col], -self._days_in_history) <= spdf_cons[self._custom_date_colname]) &
                (self._base_spdf[self._date_col] > spdf_cons[self._custom_date_colname])
            ), how='left')
            .drop(spdf_cons.billing_account_id)
            .groupby('billing_account_id', 'date')
            .agg(*self._generate_spark_cols())
        )

        return spdf_features
