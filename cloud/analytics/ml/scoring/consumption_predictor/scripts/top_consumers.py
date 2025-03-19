# pylint: disable=no-value-for-parameter
import click
from clan_tools.data_adapters.crm.CRMModelAdapter import CRMModelAdapter, upsale_to_update_leads
from pyspark.sql.functions import col, lit
import pyspark.sql.functions as F
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.spark import DEFAULT_SPARK_CONF
from datetime import timedelta, datetime
import spyt
from consumption_predictor.data_adapters.SparkFeaturesAdapter import SparkFeaturesAdapter
from clan_tools.data_adapters.crm.CRMHistoricalDataAdapter import CRMHistoricalDataAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter
import logging.config
from spyt import spark_session
import os

os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
print(os.listdir('.'))


logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


@click.command('top_consumers')
@click.option('--targets_dir')
@click.option('--crm_path')
def top_consumers(targets_dir: str, crm_path: str):

    with spark_session(yt_proxy="hahn", spark_conf_args=DEFAULT_SPARK_CONF, driver_memory='2G') as spark:
        spyt.info(spark)
        last_days = 30
        features_adapter = SparkFeaturesAdapter(
            spark, YTAdapter(token=os.environ["SPARK_SECRET"]))
        day_use = features_adapter.get_day_use()

        staff = spark.read.yt('//home/cloud_analytics/import/staff/cloud_staff/cloud_staff')\
            .filter((col('group_name') == 'Группа по привлечению новых клиентов')
                    & col('quit_at').isNull())\
            .select('login').cache()

        yt_adapter = YTAdapter(token=os.environ["SPARK_SECRET"])
        historical_data_adapter = CRMHistoricalDataAdapter(yt_adapter, spark)

        t_end = datetime.now()
        t_start = t_end - timedelta(days=last_days)
        top_consumers = day_use\
            .join(staff, on=(day_use.crm_account_owner == staff.login), how='left')\
            .filter(((col('crm_account_owner') == 'No Account Owner') | (day_use.crm_account_owner == staff.login))
                    & (col('segment').isin('Mass', 'Medium'))
                    & (col('is_var') != 'var')
                    )\
            .filter(F.to_date('event_time') > t_start) \
            .groupBy(['billing_account_id', 'account_name'])\
            .agg(F.sum(col('br_cost')).alias('total_consumption'),
                 F.last(col('crm_account_owner')).alias('crm_account_owner')
                 ) \
            .select('billing_account_id',
                    'account_name',
                    F.lit(t_start.isoformat(' ', 'seconds')).alias('t_start'),
                    F.lit(t_end.isoformat(' ', 'seconds')).alias('t_end'),
                    'crm_account_owner',
                    'total_consumption')\
            .filter(col('total_consumption') > 50000)

        crm_historical_data = historical_data_adapter.historical_preds().cache()
        filter_statuses = ['in process', 'pending',
                           'new',  'converted', 'assigned', 'recycled']
        existing_crm_preds = crm_historical_data.filter(
            F.lower(col('status')).isin(filter_statuses))
        filtered_consumers = top_consumers\
            .join(existing_crm_preds, how='left', on='billing_account_id')\
            .join(staff, how='left', on=(existing_crm_preds.user_name == staff.login))\
            .filter(staff.login.isNotNull() | existing_crm_preds.lead_id.isNull())\
            .select('billing_account_id',
                    'account_name',
                    't_start',
                    't_end',
                    col('total_consumption').alias(
                        'total_consumption_last_30_days'),
                    'crm_account_owner',
                    col('status').alias('crm_status'),
                    col('user_name').alias('crm_user_name')
                    )\
            .drop_duplicates().cache()

        def _descr():
            return F.concat(lit('Account "'),
                            col('billing_account_id'),
                            lit('" has consumption '),
                            F.round(col('total_consumption_last_30_days'), 2),
                            lit(f'rub over last {last_days} days'))

        leads = filtered_consumers\
            .select(col('billing_account_id'),
                    col("total_consumption_last_30_days").alias('sort_column'),
                    _descr().alias('description'))

        crm_model_adapter = CRMModelAdapter(yt_adapter, spark,
                                            predictions=leads,
                                            lead_source='Consumed more than 40k over last 30 days',
                                            leads_daily_limit=10)

        table_name = datetime.now().strftime('%Y-%m-%dT%H:%M:%S')

        filtered_consumers.write.optimize_for(
            "scan").yt(f'{targets_dir}/{table_name}')

        filtered_leads, _ = crm_model_adapter.save_to_crm()
        upsale_to_update_leads(filtered_leads).write.yt(
            crm_path, mode='append')


if __name__ == '__main__':
    top_consumers()
