import typing as tp
from datetime import datetime

import pyspark.sql.functions as F
from pyspark.sql.functions import col
from pyspark.sql.session import SparkSession
from pyspark.sql.dataframe import DataFrame as SparkDataFrame

from clan_tools.data_adapters.YTAdapter import YTAdapter
from ml_flows.features.BaseFeatureAdapter import KNOWN_AGGREGATIONS, BaseFeatureAdapter, take_table


class PaymentsFeaturesAdapter(BaseFeatureAdapter):

    PAYMENTS_FOLDER = '//home/cloud_analytics/ml/ml_model_features/by_baid/payments'

    def __init__(self, spark: SparkSession, yt_adapter: YTAdapter,
                 base_spdf: SparkDataFrame, date_col: str = 'date',
                 functions_to_columns: tp.Optional[tp.Dict[str, tp.List[str]]] = None,
                 days_in_history: int = 30) -> None:
        super().__init__(spark, yt_adapter, base_spdf, date_col, functions_to_columns, days_in_history)

        per_s = datetime.strptime(self._start_date, '%Y-%m-%d').strftime('%Y-%m')
        per_e = datetime.strptime(self._end_date, '%Y-%m-%d').strftime('%Y-%m')

        self._tables = [f'{self.PAYMENTS_FOLDER}/{tbl}' for tbl in self._yt_adapter.yt.list(self.PAYMENTS_FOLDER) if take_table(tbl, per_s, per_e)]

    def get_double_features(self) -> SparkDataFrame:
        spdf_paym = self._spark.read.option('mergeschema', 'true').yt(*self._tables).withColumn(self._custom_date_colname, col('date')).drop('date')

        # DataFrame default columns
        if self._functions_to_columns is None:
            spdf_paym_double_features = ['paid_amount']
            self._functions_to_columns = {fn: spdf_paym_double_features for fn in KNOWN_AGGREGATIONS}

        # final dataset
        spdf_features = (
            self._base_spdf
            .join(spdf_paym, on=(
                (self._base_spdf.billing_account_id == spdf_paym.billing_account_id) &
                (F.date_add(self._base_spdf[self._date_col], -self._days_in_history) <= spdf_paym[self._custom_date_colname]) &
                (self._base_spdf[self._date_col] > spdf_paym[self._custom_date_colname])
            ), how='left')
            .drop(spdf_paym.billing_account_id)
            .groupby('billing_account_id', 'date')
            .agg(*self._generate_spark_cols())
        )

        return spdf_features

    def get_category_features(self) -> SparkDataFrame:
        spdf_paym = (
            self._spark.read.yt(*self._tables)
            .select(
                'billing_account_id',
                'date',
                'usage_status',
                'person_type',
                'currency',
                'state',
                'is_fraud',
                'is_subaccount',
                'is_suspended_by_antifraud',
                'is_isv',
                'is_var',
                'crm_account_id',
                'partner_manager',
                'segment',
                'payment_type'
            )
        )

        spdf_features = (
            self._base_spdf
            .join(spdf_paym, on=(
                (self._base_spdf.billing_account_id == spdf_paym.billing_account_id) &
                (F.date_add(self._base_spdf[self._date_col], -1) == spdf_paym.date)
            ), how='left')
            .drop(spdf_paym.billing_account_id, spdf_paym.date)
        )
        return spdf_features

    def get_quantitative_features(self) -> SparkDataFrame:
        spdf_paym = (
            self._spark.read.yt(*self._tables)
            .select(
                'billing_account_id',
                'date',
                'days_from_created',
                'days_since_last_payment'
            )
        )

        spdf_features = (
            self._base_spdf
            .join(spdf_paym, on=(
                (self._base_spdf.billing_account_id == spdf_paym.billing_account_id) &
                (F.date_add(self._base_spdf[self._date_col], -1) == spdf_paym.date)
            ), how='left')
            .drop(spdf_paym.billing_account_id, spdf_paym.date)
        )
        return spdf_features
