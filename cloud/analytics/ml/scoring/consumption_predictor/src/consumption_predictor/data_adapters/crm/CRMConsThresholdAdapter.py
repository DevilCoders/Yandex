from datetime import datetime, timedelta
from pyspark.sql.session import SparkSession
import pyspark.sql.functions as F
from pyspark.sql.functions import col, lit
from clan_tools.utils.spark import prepare_for_yt
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.data_adapters.crm.CRMHistoricalDataAdapter import CRMHistoricalDataAdapter


def max_by(x, y):
    return F.expr(f'max_by({x}, {y})')


class CRMConsThresholdAdapter:
    def __init__(self, spark: SparkSession,
                 yt_adapter: YTAdapter,
                 crm_table_to: str,
                 days_period: int = 90,
                 consumption_threshold: int = 150000):
        self._spark = spark
        self._days_period = days_period
        self._consumption_threshold = consumption_threshold
        self._historical_data_adapter = CRMHistoricalDataAdapter(yt_adapter, spark)
        self._crm_table_to = crm_table_to

    def _get_ba_over_threshold(self):
        ba_over_threshold = self._spark.read.yt("//home/cloud_analytics/cubes/acquisition_cube/cube")\
            .filter((col('event') == 'day_use') & (col('ba_usage_status') != 'service')
                    & col('segment').isin('Mass', 'Medium')
                    & (col('sales_name') == 'unmanaged')
                    & (col('ba_state')=='active')
                    & (F.to_date('event_time') > (datetime.now() - timedelta(days=self._days_period)))) \
            .groupby('billing_account_id').agg(F.sum(col('real_consumption')).alias('paid_cons')) \
            .where(col('paid_cons')>self._consumption_threshold)\
            .select('billing_account_id', 'paid_cons')
        return ba_over_threshold

    @prepare_for_yt
    def restore_recycled(self):
        date_from = datetime.now() - timedelta(days=self._days_period)
        ba_over_threshold = self._get_ba_over_threshold().cache()
        crm_historical_data = self._historical_data_adapter.historical_preds().cache()
        recycled_ba = (
            crm_historical_data
            .filter(col('lead_source').like('%30k%') | col('lead_source').like('%50k%'))
            .groupby('billing_account_id')
            .agg(F.max('date_modified').alias("date_modified"), max_by('status', 'date_modified').alias("last_status"))
            .filter((F.lower(col("last_status")) == 'recycled') & (F.from_unixtime(col('date_modified')) < date_from))
            .cache()
        )
        qualified_ba = (
            crm_historical_data
            .filter(col('lead_source_crm')!='trial')
            .filter(col('status')=='Converted')
            .filter(~col('billing_account_id').isNull())
            .select('billing_account_id')
        )
        recycled = crm_historical_data.join(recycled_ba, on=['billing_account_id', 'date_modified'], how='inner')

        # recycled = crm_historical_data.filter((F.lower(col('status')) == 'recycled')
        #                                       & (col('lead_source').like('%30k%') | col('lead_source').like('%50k%'))
        #                                       & (F.from_unixtime(col('date_modified')) < date_from))
        # assigned = crm_historical_data.filter((F.lower(col('status')) != 'recycled')
        #                                       & (col('lead_source').like('%30k%') | col('lead_source').like('%50k%'))
        #                                       & (F.from_unixtime(col('date_modified')) >= date_from))
        # recycled = recycled.join(assigned, on='billing_account_id', how='leftanti').drop_duplicates()

        recycled_to_restore = recycled.join(ba_over_threshold, how='leftsemi', on='billing_account_id')\
                                .withColumn('description', lit('Restored from recycled, since consumed more'+
                                            f'than {self._consumption_threshold} over {self._days_period} days'))

        max_date = recycled_to_restore.groupby('billing_account_id').agg(F.max(col('date_entered')).alias('date_entered'))
        filtered_restore = (
            recycled_to_restore
            .join(max_date, on=['billing_account_id', 'date_entered'], how='leftsemi')
            .join(qualified_ba, on='billing_account_id', how='leftanti')
        )
        return filtered_restore
