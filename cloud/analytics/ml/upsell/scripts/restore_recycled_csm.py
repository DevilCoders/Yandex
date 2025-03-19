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
from typing import List
from pyspark.sql.session import SparkSession

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"


def get_current_upsell_staff_logins(spark: SparkSession) -> List[str]:
    """Provides list of logins for current upsell team in staff

    Returns
    ------
    list
        list of logins
    """
    staff_info_path = '//home/cloud-dwh/data/prod/ods/staff/persons'
    staff_pii_info_path = '//home/cloud-dwh/data/prod/ods/staff/PII/persons'
    current_staff = (
        spark.read.yt(staff_info_path)
        .join(
            spark.read.yt(staff_pii_info_path),
            on='staff_user_id', how='left'
        )
        .filter(col('department_id').isin([16682, 13473]))
        .filter(~col('official_is_dismissed'))
        .select('staff_user_login')
    )
    staff_list = current_staff.toPandas()['staff_user_login'].tolist()
    return staff_list


@click.command()
@click.option('--leads_table', default="//home/cloud_analytics/export/crm/update_call_center_leads/update_leads_test")
@click.option('--leads_table_history', default="//home/cloud_analytics/ml/upsell/csm_restore_leads/csm_restore_history_test")
def generate_leads(leads_table: str, leads_table_history: str):

    logger.info('Starting source project')
    yt_token = os.environ["SPARK_SECRET"] if "SPARK_SECRET" in os.environ else os.environ["YT_TOKEN"]
    yt_adapter = YTAdapter(yt_token)
    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_SMALL, driver_memory='2G') as spark:

        spdf_res = Generate_CSM_Leads(spark, yt_adapter, consumption_threshold=150000, days_period=90).restore_recycled()
        contact_info_path = '//home/cloud_analytics/import/crm/leads/contact_info'
        spdf_contacts = spark.read.yt(contact_info_path)
        leads_timestamp = int((datetime.now()+timedelta(hours=3)).timestamp())  # adapting to Moscow timezone (on system is UTC+0)
        actual_logins = get_current_upsell_staff_logins(spark)

        spdf_res_leads = (
            spdf_res
            .join(spdf_contacts, on=['billing_account_id'], how="left")
            .withColumn('null_prev_names', spdf_res.first_name.isNull() & spdf_res.last_name.isNull())
            .withColumn('crm_phone', F.regexp_replace(spdf_res.phone, '[+\\-\\s\\(\\)]', ''))
            .select(
                lit(leads_timestamp).alias('Timestamp'),
                lit(None).astype('string').alias('CRM_Lead_ID'),
                F.concat(lit('["'), "billing_account_id", lit('"]')).alias("Billing_account_id"),
                lit(None).astype('string').alias('Status'),
                col('description').alias('Description'),
                F.when(col('user_name').isin(actual_logins), col('user_name')).otherwise(lit('admin')).alias('Assigned_to'),
                F.when(col('null_prev_names'), spdf_contacts.first_name).otherwise(spdf_res.first_name).alias('First_name'),
                F.when(col('null_prev_names'), spdf_contacts.last_name).otherwise(spdf_res.last_name).alias('Last_name'),
                spdf_contacts.phone.alias('Phone_1'),
                F.when(spdf_contacts.phone==col('crm_phone'), lit(None).astype('string')).otherwise(col('crm_phone')).alias('Phone_2'),
                F.concat(lit('["'), spdf_res.email, lit('"]')).alias('Email'),
                lit('upsell').alias('Lead_Source'),
                col('lead_source').alias('Lead_Source_Description'),
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
