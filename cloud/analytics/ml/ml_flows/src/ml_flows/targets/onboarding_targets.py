from abc import abstractmethod
import typing as tp
from datetime import date, datetime

import pyspark.sql.functions as F
from pyspark.sql.functions import col, lit
from pyspark.sql.session import SparkSession
from pyspark.sql.dataframe import DataFrame as SparkDataFrame

from clan_tools.data_adapters.YTAdapter import YTAdapter


class OnboardingBaseTarget:
    """Parent target class with pre-defined functions and arguments"""

    PAYMENTS_FOLDER = '//home/cloud_analytics/ml/ml_model_features/by_baid/payments'

    def __init__(self, spark: SparkSession, yt_adapter: YTAdapter, date_from: str = '2021-01-01', date_to: tp.Optional[str] = None,
                 tag: str = 'ONB', day_to_score_in: int = 30, days_in_future: int = 30, amount_for_positive: float = 1000) -> None:
        self._spark = spark
        self._yt_adapter = yt_adapter
        self._date_from = date_from
        self._date_to = date_to or datetime.now().strftime('%Y-%m-%d')
        self._day_to_score_in = day_to_score_in
        self._days_in_future = days_in_future
        self._amount_for_positive = amount_for_positive
        self._source_tables = self._get_source_tables(self._get_source_folder())
        self._base_tables = self._get_source_tables(self.PAYMENTS_FOLDER)
        self._tag = tag

    def _get_source_tables(self, folder: str) -> tp.List[str]:
        period_start = datetime.strptime(self._date_from, '%Y-%m-%d').strftime('%Y-%m')
        period_end = datetime.strptime(self._date_to, '%Y-%m-%d').strftime('%Y-%m')

        def period_between(dt_compare: tp.Union[str, datetime, date],
                           dt_start: tp.Union[str, datetime, date], dt_end: tp.Union[str, datetime, date]) -> bool:
            result = dt_start <= dt_compare <= dt_end  # type: ignore
            return result

        tables = [path for path in self._yt_adapter.yt.list(folder) if period_between(path, period_start, period_end)]
        source_tables = [f'{folder}/{table}' for table in tables]
        return source_tables

    @abstractmethod
    def _get_source_folder(self) -> str:
        pass

    def _get_base(self) -> SparkDataFrame:
        spdf_base = (
            self._spark.read.yt(*self._base_tables)
            .filter(col('person_type').rlike('individual'))
            .filter(col('state') == 'active')
            .filter(col('is_suspended_by_antifraud') == lit(False))
            .filter(col('is_subaccount') == lit(False))
            .filter(col('is_var') == lit(False))
            .filter(col('segment') == 'Mass')
            .filter(col('crm_account_id').isNull())
            .filter(col('days_from_created') == self._day_to_score_in)
            .filter(col('date') >= self._date_from)
            .filter(col('date') <= self._date_to)
            .filter(F.date_add('date', self._days_in_future) < datetime.now().strftime('%Y-%m-%d'))
            .select('billing_account_id', col('date').alias('rep_date'))
            .cache()
        )
        return spdf_base

    @abstractmethod
    def generate_spark_dataframe(self) -> SparkDataFrame:
        pass

    def generate_spark_dataframe_name(self) -> str:
        return f'{self._tag}_{type(self).__name__}_score_{self._day_to_score_in}_future_{self._days_in_future}_amount_{self._amount_for_positive}'


class PaymentsTarget(OnboardingBaseTarget):
    """Class for payments-based target generation"""

    def _get_source_folder(self) -> str:
        return self.PAYMENTS_FOLDER

    def generate_spark_dataframe(self) -> SparkDataFrame:
        spdf_base = self._get_base()

        spdf_data = (
            self._spark.read.yt(*self._source_tables)
            .select('billing_account_id', 'date', 'paid_amount')
        )

        spdf_target = (
            spdf_base
            .join(spdf_data, on=(
                (spdf_base.billing_account_id == spdf_data.billing_account_id) &
                (F.date_add(spdf_base.rep_date, 1) <= spdf_data.date) &
                (F.date_add(spdf_base.rep_date, self._days_in_future) >= spdf_data.date)
            ), how='left')
            .groupby(spdf_base.billing_account_id, 'rep_date')
            .agg(F.coalesce(F.sum('paid_amount'), lit(0.0)).alias('paid_amount'))
            .select(
                'billing_account_id',
                col('rep_date').alias('date'),
                (col('paid_amount') > self._amount_for_positive).astype('bigint').alias('target')
            )
            .cache()
        )
        return spdf_target


class ConsumptionTarget(OnboardingBaseTarget):
    """Class for consumption-based target generation"""

    CONSUMPTION_FOLDER = '//home/cloud_analytics/ml/ml_model_features/by_baid/consumption'

    def _get_source_folder(self) -> str:
        return self.CONSUMPTION_FOLDER

    def generate_spark_dataframe(self) -> SparkDataFrame:
        spdf_base = self._get_base()

        spdf_data = (
            self._spark.read.yt(*self._source_tables)
            .select('billing_account_id', 'date', 'billing_record_total_rub')
        )

        spdf_target = (
            spdf_base
            .join(spdf_data, on=(
                (spdf_base.billing_account_id == spdf_data.billing_account_id) &
                (F.date_add(spdf_base.rep_date, 1) <= spdf_data.date) &
                (F.date_add(spdf_base.rep_date, self._days_in_future) >= spdf_data.date)
            ), how='left')
            .groupby(spdf_base.billing_account_id, 'rep_date')
            .agg(F.coalesce(F.sum('billing_record_total_rub'), lit(0.0)).alias('consumed_amount'))
            .select(
                'billing_account_id',
                col('rep_date').alias('date'),
                (col('consumed_amount') > self._amount_for_positive).astype('bigint').alias('target')
            )
            .cache()
        )
        return spdf_target


class MarketoTarget(OnboardingBaseTarget):

    def __init__(self, spark: SparkSession, yt_adapter: YTAdapter, date_from: str = '2021-01-01', date_to: tp.Optional[str] = None,
                 tag: str = 'ONB', day_to_score_in: int = 30) -> None:
        self._spark = spark
        self._yt_adapter = yt_adapter
        self._date_from = date_from
        self._date_to = date_to or datetime.now().strftime('%Y-%m-%d')
        self._day_to_score_in = day_to_score_in
        self._days_in_future = 1
        self._base_tables = self._get_source_tables(self.PAYMENTS_FOLDER)
        self._tag = tag

    def _get_source_folder(self) -> str:
        raise AttributeError('`_get_source_folder` is not used in this class')

    def generate_spark_dataframe(self) -> SparkDataFrame:
        spdf_base = self._get_base()
        spdf_marketo_output = self._spark.read.yt('//home/cloud_analytics/call_center/output/marketo_output')
        spdf_leads = self._spark.read.yt('//home/cloud_analytics/kulaga/leads_cube')

        spdf_marketo_target = (
            spdf_marketo_output
            .join(spdf_leads, on=(spdf_marketo_output.task_id == spdf_leads.lead_id), how='left')
            .filter(spdf_marketo_output.status == 'success')
            .select(
                'billing_account_id',
                F.coalesce('is_business', lit(False)).astype('bigint').alias('target'),
            )
            .join(spdf_base, on='billing_account_id', how='inner')
            .select('billing_account_id', col('rep_date').alias('date'), 'target')
            .cache()
        )

        return spdf_marketo_target

    def generate_spark_dataframe_name(self) -> str:
        return f'{self._tag}_{type(self).__name__}_score_{self._day_to_score_in}'
