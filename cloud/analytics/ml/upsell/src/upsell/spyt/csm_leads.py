import logging.config
from datetime import datetime, timedelta

import pyspark.sql.types as T
import pyspark.sql.functions as F
from pyspark.sql.functions import col, lit
from pyspark.sql.session import SparkSession

from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.data_adapters.crm.CRMHistoricalDataAdapter import CRMHistoricalDataAdapter

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def max_by(x, y):
    return F.expr(f'max_by({x}, {y})')


class Generate_CSM_Leads:
    dm_yc_consumption_path = '//home/cloud-dwh/data/prod/cdm/dm_yc_consumption'
    dm_crm_tags = '//home/cloud-dwh/data/prod/cdm/dm_ba_crm_tags'

    def __init__(self, spark: SparkSession, yt_adapter: YTAdapter, days_period: int = 90, consumption_threshold: int = 150000) -> None:
        self.spark = spark
        self.consumption_threshold = consumption_threshold
        self.historical_data_adapter = CRMHistoricalDataAdapter(yt_adapter, spark)
        self.days_period = days_period
        self.today = datetime.now().date()
        self.date_from = self.today - timedelta(days_period)

    def get_ba_over_threshold(self):
        crm_tags_filter = (
            self.spark.read.yt(self.dm_crm_tags)
            .select(
                'billing_account_id',
                'account_owner_current',
                'usage_status_current',
                'segment_current',
                'state_current',
                'is_suspended_by_antifraud_current',
                'is_var_current'
            )
            .distinct()
        )
        ba_over_threshold = (
            self.spark.read.yt(self.dm_yc_consumption_path)
            .filter(F.to_date('billing_record_msk_date') >= self.date_from)
            .filter(F.to_date('billing_record_msk_date') < self.today)
            .groupby('billing_account_id')
            .agg(F.sum('billing_record_real_consumption_rub').alias('paid_cons'))
            .filter(col('paid_cons')>self.consumption_threshold)
            .join(crm_tags_filter, on='billing_account_id', how='left')
            .filter(col('usage_status_current')=='paid')
            .filter(col('segment_current').isin(['Mass', 'Medium']))
            .filter(col('account_owner_current')=='No Account Owner')
            .filter(col('state_current')=='active')
            .filter(~col('is_suspended_by_antifraud_current'))
            .filter(~col('is_var_current'))
            .select('billing_account_id', 'paid_cons')
            .cache()
        )

        rows_num = ba_over_threshold.count()
        days_num = self.days_period
        thrsh = self.consumption_threshold
        print(f'Loaded BAs with consumption exceeded {thrsh} RUB in {days_num} days period: {rows_num} row(s)')

        return ba_over_threshold

    def restore_recycled(self):
        ba_over_threshold = self.get_ba_over_threshold()
        crm_historical_data = self.historical_data_adapter.historical_preds().cache()
        recycled_ba = (
            crm_historical_data
            .filter(col('lead_source').like('%30k%') | col('lead_source').like('%50k%'))
            .groupby('billing_account_id')
            .agg(
                F.max('date_modified').alias("date_modified"),
                max_by('status', 'date_modified').alias("last_status")
            )
            .filter(F.lower(col("last_status")) == 'recycled')
            .filter(F.from_unixtime(col('date_modified').astype(T.LongType())/1000000) < self.date_from)
            .cache()
        )
        qualified_ba = (
            crm_historical_data
            .filter(col('lead_source_crm')!='trial')
            .filter(col('status')=='Converted')
            .filter(~col('billing_account_id').isNull())
            .select('billing_account_id')
        )
        recycled = (
            crm_historical_data
            .join(recycled_ba, on=['billing_account_id', 'date_modified'], how='inner')
        )
        recycled_to_restore = (
            recycled
            .join(ba_over_threshold, how='leftsemi', on='billing_account_id')
            .withColumn('description',
                        lit('Restored from recycled, since consumed more'+
                            f'than {self.consumption_threshold} over {self.days_period} days'))
        )
        max_date = (
            recycled_to_restore
            .groupby('billing_account_id')
            .agg(F.max(col('date_entered')).alias('date_entered'))
        )
        filtered_restore = (
            recycled_to_restore
            .join(max_date, on=['billing_account_id', 'date_entered'], how='leftsemi')
            .join(qualified_ba, on='billing_account_id', how='leftanti')
        )
        return filtered_restore

    def top_cunsomers(self):
        ba_over_threshold = self.get_ba_over_threshold()
        crm_wo_trial = (
            self.historical_data_adapter
            .historical_preds()
            .filter(col('lead_source_crm')!='trial')
            .cache()
        )
        top_cunsomers = (
            ba_over_threshold
            .join(crm_wo_trial, on='billing_account_id', how='leftanti')
            .withColumn(
                'description',
                F.concat(
                    lit('Account "'),
                    col('billing_account_id'),
                    lit('" has consumption '),
                    F.round(col('paid_cons'), 2),
                    lit(f'rub over last {self.days_period} days')
                )
            )
        )
        return top_cunsomers
