# pylint: disable=no-value-for-parameter
import os
import click
import logging.config
from datetime import datetime, timedelta
from spyt import spark_session
import pyspark.sql.functions as F
from pyspark.sql.functions import col, lit
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.utils.spark import SPARK_CONF_SMALL
from clan_tools.utils.spark import safe_append_spark
from upsell.spyt.csm_leads import Generate_CSM_Leads

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"


@click.command()
@click.option('--leads_table', default="//home/cloud_analytics/export/crm/update_call_center_leads/update_leads_test")
@click.option('--leads_table_history', default="//home/cloud_analytics/ml/upsell/csm_top_cunsomers_leads/csm_top_cunsomers_history_test")
@click.option('--exclude_csm_leads', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/crm/upsell/csm_history_test")
def generate_leads(leads_table: str, leads_table_history: str, exclude_csm_leads: str):

    logger.info('Starting source project')
    yt_token = os.environ["SPARK_SECRET"] if "SPARK_SECRET" in os.environ else os.environ["YT_TOKEN"]
    yt_adapter = YTAdapter(yt_token)
    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_SMALL, driver_memory='2G') as spark:

        contact_info_path = '//home/cloud_analytics/import/crm/leads/contact_info'
        spdf_contacts = spark.read.yt(contact_info_path)
        leads_timestamp = int((datetime.now()+timedelta(hours=3)).timestamp())  # adapting to Moscow timezone (on system is UTC+0)

        spdf_res = Generate_CSM_Leads(spark,
                                      yt_adapter,
                                      consumption_threshold=40000,
                                      days_period=30).top_cunsomers()

        leads_to_exclude = (
            spark.read.yt(exclude_csm_leads)
            .select(F.regexp_replace('Billing_account_id', r'[\[\]"]', '').alias('billing_account_id'))
            .distinct()
        )
        spdf_res_leads = (
            spdf_res
            .join(spdf_contacts, on='billing_account_id', how="left")
            .join(leads_to_exclude, on='billing_account_id', how="leftanti")
            .select(
                lit(leads_timestamp).alias('Timestamp'),
                lit(None).astype('string').alias('CRM_Lead_ID'),
                F.concat(lit('["'), "billing_account_id", lit('"]')).alias("Billing_account_id"),
                lit(None).astype('string').alias('Status'),
                col('description').alias('Description'),
                lit('admin').alias('Assigned_to'),
                col('first_name').alias('First_name'),
                col('last_name').alias('Last_name'),
                col('phone').alias('Phone_1'),
                lit(None).astype('string').alias('Phone_2'),
                F.concat(lit('["'), col('email'), lit('"]')).alias('Email'),
                lit('upsell').alias('Lead_Source'),
                lit('Consumed more than 40k over last 30 days').alias('Lead_Source_Description'),
                lit(None).astype('string').alias('Callback_date'),
                lit(None).astype('string').alias('Last_communication_date'),
                lit(None).astype('string').alias('Promocode'),
                lit(None).astype('string').alias('Promocode_sum'),
                lit(None).astype('string').alias('Notes'),
                lit(None).astype('string').alias('Dimensions'),
                lit(None).astype('string').alias('Tags'),
                lit('').alias('Timezone'),
                col("display_name").alias('Account_name')
            )
            .coalesce(1)
        )

        safe_append_spark(yt_adapter=yt_adapter, spdf=spdf_res_leads, path=leads_table, mins_to_wait=60)
        safe_append_spark(yt_adapter=yt_adapter, spdf=spdf_res_leads, path=leads_table_history, mins_to_wait=60)

    yt_adapter.optimize_chunk_number(leads_table)
    yt_adapter.optimize_chunk_number(leads_table_history)

if __name__ == '__main__':
    generate_leads()
