import logging.config
import os
import pyspark.sql.functions as F
import pyspark.sql.types as T
from pyspark.sql.window import Window
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.spark import SPARK_CONF_SMALL
from pyspark.sql.functions import col
from spyt import spark_session
from ltv.utils import config, spark_melt
# if you want to run local use spark-submit or something


os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"


logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)

def previous_monday(date):
    return F.date_sub(F.next_day(date, "monday"), 7)


def main():
    result_path = '//home/cloud_analytics/ml/ltv/data'
    yc_consumption = '//home/cloud-dwh/data/prod/cdm/dm_yc_consumption'
    marketing_attribution = '//home/cloud-dwh/data/prod/cdm/marketing/dm_marketing_attribution'

    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_SMALL, driver_memory='2G') as spark:

        
        cube =  spark.read.yt(yc_consumption)
        
        market_attr = (
            spark.read.yt('//home/cloud-dwh/data/prod/cdm/marketing/dm_marketing_attribution')
            .select(
                'billing_account_id',
                'channel',
                'event_exp_7d_half_life_time_decay_weight',
                'event_first_weight',
                'event_last_weight',
                'event_u_shape_weight',
                'event_uniform_weight'
            )
        )

        market_attr = spark_melt(
            market_attr,
            id_vars=['billing_account_id', 'channel'],
            value_vars=[
                'event_exp_7d_half_life_time_decay_weight',
                'event_first_weight',
                'event_last_weight',
                'event_u_shape_weight',
                'event_uniform_weight'],
            var_name='channel_weight_name',
            value_name='channel_weight_value'
        )

        cube = (
            cube
            .join(market_attr, on='billing_account_id')
            .select(
                'crm_segment_current',
                'billing_account_usage_status_current',
                'billing_account_state',
                'billing_record_msk_date',
                'billing_account_id',
                'channel',
                'channel_weight_name',
                'billing_account_month_cohort',
                (col('billing_record_real_consumption_rub_vat')*col('channel_weight_value')).alias('billing_record_real_consumption_rub_vat')
            )
        )
            
        consumption_data = (
            cube
            .filter(col('crm_segment').isNull() | col('crm_segment').isin(config['segments']))
            .filter(col('billing_account_state') != 'suspended')
            .filter(col('billing_account_state') != 'inactive')
            .filter(F.to_date('billing_record_msk_date') >= config['start_date'])
            .filter(F.to_date('billing_record_msk_date') < config['end_date'])
            .select(
                'billing_account_id',
                'channel',
                'channel_weight_name',
                F.to_date('billing_record_msk_date').alias('event_date'),
                F.date_trunc('mon', 'billing_record_msk_date').alias('event_month'),
                col('billing_record_real_consumption_rub_vat').alias('real_consumption'),
                col('billing_account_month_cohort').alias('start_month')          
            )
        )
        
        check = (
            consumption_data
            .filter(col('real_consumption') > 0)
            .groupby('billing_account_id', 'event_month')
            .agg(F.sum('real_consumption').alias('month_check'))
            .select('month_check')
        ).toPandas()
        pp95_check = check['month_check'].quantile(0.95)
            
        
        check = (
            consumption_data
            .filter(col('real_consumption') > 0)
            .groupby('billing_account_id', 'event_month')
            .agg(F.sum('real_consumption').alias('month_check'))
            .filter(col('month_check') > pp95_check)
            .select('billing_account_id')
            .distinct()
        )
        
        data_with_dates = (
            consumption_data
            .join(check, on='billing_account_id', how='left_anti')
            .withColumn("event_week", previous_monday("event_date"))
        )

        window = Window.orderBy("event_week").partitionBy(['start_month', 'channel', 'channel_weight_name']).rowsBetween(Window.unboundedPreceding, Window.currentRow)
        grouped = (
            data_with_dates
            .groupby("event_week", 'start_month', 'channel', 'channel_weight_name')
            .agg(
                (F.sum(col('real_consumption'))).alias('week_consumption'),
                F.countDistinct('billing_account_id').alias('people_count')
            )
            .withColumn('week', F.sum(F.lit(1)).over(window))
            .select('week_consumption', 'people_count', 'channel', 'channel_weight_name', 'week', 'start_month', col('event_week').cast(T.StringType()).alias('event_week'))
        )

        grouped.coalesce(1).write.yt(result_path, mode='overwrite')


if __name__ == '__main__':
    main()
