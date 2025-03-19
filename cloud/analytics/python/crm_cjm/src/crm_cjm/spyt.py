import pyspark.sql.types as T
import pyspark.sql.functions as F

from pyspark.sql.functions import col, lit
from pyspark.sql.session import SparkSession
from pyspark.sql.dataframe import DataFrame as SparkDataFrame
from pyspark.sql.column import Column as SparkColumn
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.data_adapters.crm.CRMHistoricalDataAdapter import CRMHistoricalDataAdapter


def min_by(colname_min, colname_by):
    return F.expr(f'Min_By(`{colname_min}`, `{colname_by}`)')


def max_by(colname_max, colname_by):
    return F.expr(f'Max_By(`{colname_max}`, `{colname_by}`)')


class CjmTable:
    """Class for collecting dash data
    """
    ba_crm_tags = '//home/cloud-dwh/data/prod/cdm/dm_ba_crm_tags'
    balance_contracts = '//home/cloud_analytics/import/balance/contracts'
    contact_info = '//home/cloud_analytics/import/crm/leads/contact_info'
    dm_yc_consumption = '//home/cloud-dwh/data/prod/cdm/dm_yc_consumption'

    def __init__(self, spark: SparkSession, yt_adapter: YTAdapter) -> None:
        self.spark = spark
        self.yt_adapter = yt_adapter

    @staticmethod
    def _get_date(colname: str) -> SparkColumn:
        uint_col = (col(colname).astype(T.LongType()) / 10**6).astype(T.LongType())
        ts_col = F.to_timestamp(uint_col)
        str_col = F.concat(F.date_format(ts_col, 'yyyy-MM-dd'), lit('T'),  F.date_format(ts_col, 'hh:mm:ss'))
        return str_col

    def get_crm_historical_data(self) -> SparkDataFrame:
        crm_historical_data_adapter = CRMHistoricalDataAdapter(self.yt_adapter, self.spark)
        crm_historical_data = (
            crm_historical_data_adapter.historical_preds()
            .filter(col('billing_account_id').isNotNull())
            .select(
                'lead_id',
                self._get_date('date_entered').alias('date_created'),
                'billing_account_id',
                col('lead_source_crm').alias('lead_source'),
                col('lead_source').alias('lead_source_description')
            )
            .distinct()
        )
        return crm_historical_data.cache()

    def _get_account_name(self) -> SparkDataFrame:
        spdf_names = (
            self.spark.read.yt(self.contact_info)
            .select('billing_account_id', 'display_name')
        )
        return spdf_names

    def _get_first_paid_date(self) -> SparkDataFrame:
        spdf_first_paid_date = (
            self.spark.read.yt(self.dm_yc_consumption)
            .filter(col('billing_record_expense') > 0.0)
            .groupby('billing_account_id')
            .agg(F.min('billing_record_msk_date').alias('first_paid_date'))
        )
        return spdf_first_paid_date

    def get_crm_cjm(self) -> SparkDataFrame:

        spdf_balance_contracts = (
            self.spark.read.yt(self.balance_contracts)
            .select(
                col('contract_projects').alias('billing_account_id'),
                'person_inn'
            )
            .join(self.spark.read.yt(self.contact_info), on='billing_account_id', how='left')
            .select(
                'billing_account_id',
                F.coalesce(col('person_inn'), col('inn')).alias('inn')
            )
        )

        spdf_leads = (
            self.get_crm_historical_data()
            .groupby('billing_account_id')
            .agg(
                F.concat_ws('; ', F.collect_list('lead_id')).alias('leads')
            )
        )

        spdf_crm_cjm = (
            self.spark.read.yt(self.ba_crm_tags)
            .groupby('billing_account_id', 'billing_account_name')
            .agg(
                F.min('date').alias('created_date'),
                max_by('state_current', 'date').alias('state'),
                max_by('crm_account_id', 'date').alias('crm_account_id'),
                max_by('account_owner_current', 'date').alias('crm_account_owner'),
                max_by('crm_account_name', 'date').alias('crm_account_name'),
                max_by('is_subaccount', 'date').alias('is_subaccount'),
                max_by('segment_current', 'date').alias('crm_segment_current'),
                max_by('person_type_current', 'date').alias('ba_person_type_current'),
                max_by('usage_status_current', 'date').alias('ba_usage_status_current'),
            )
            .filter(col('ba_usage_status_current').isin(['paid', 'trial']))
            .join(spdf_balance_contracts, on='billing_account_id', how='left')
            .join(spdf_leads, on='billing_account_id', how='left')
            .join(self._get_account_name(), on='billing_account_id', how='left')
            .join(self._get_first_paid_date(), on='billing_account_id', how='left')
            .withColumn('leads', F.coalesce(col('leads'), lit('')))
            .distinct()
        )

        return spdf_crm_cjm


__all__ = ['CjmTable']
