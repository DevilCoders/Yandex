import logging.config
import os
import click
import pandas as pd
import pyspark.sql.functions as F
import pyspark.sql.types as T
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.spark import SPARK_CONF_SMALL
from pyspark.sql.functions import col, lit
from tracker_scoring.preprocessing import add_brackets, company_size_udf, phone_decorator_udf
from spyt import spark_session
# if you want to run local use spark-submit or something


os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"


logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


@click.command()
@click.option('--yandex_employees', default="//home/cloud_analytics/ml/tracker_scoring/yndx_passports")
@click.option('--dm_crm_leads_path', default="//home/cloud-dwh/data/prod/raw/mysql/crm/cloud8_leads")
@click.option('--puids_path', default="//home/ecosystem/_export_/cloud/CLOUDANA-1554/puids_result")
@click.option('--crm_export_path', default="//home/cloud_analytics/ml/tracker_scoring/crm_export")
@click.option('--leads_path', default="//home/cloud_analytics/export/crm/update_call_center_leads/update_leads")
@click.option('--result_path', default="//home/cloud_analytics/export/crm/update_call_center_leads/update_leads")
def main(yandex_employees: str, dm_crm_leads_path: str, puids_path: str, crm_export_path: str, leads_path: str, result_path: str):

    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_SMALL, driver_memory='2G') as spark:


        leads = (
            spark.read.yt(crm_export_path)
            .join(
                spark.read.yt(puids_path).select(col('*'), col('puid').alias('passport_uid')),
                on='passport_uid')
            .join(
                spark.read.yt(yandex_employees).select('passport_uid'),
                on='passport_uid',
                how='leftanti')
            .filter(col('secure_phone_number').isNotNull())
        )

        companies = (
            leads
            .groupby(col('resource_id'))
            .agg(
                F.max('secure_phone_number').alias('secure_phone_number'),
                F.max('display_name').alias('display_name'),
                F.max('lastname').alias('lastname'),
                F.max('firstname').alias('firstname'),
                F.max('email').alias('email'),
                F.max('size').alias('size')
            )
            .groupby(col('secure_phone_number'))
            .agg(
                F.max('display_name').alias('display_name'),
                F.max('lastname').alias('lastname'),
                F.max('firstname').alias('firstname'),
                F.max('email').alias('email'),
                F.max('size').alias('size')
            )
        )

        naming = (
            companies
            .filter(~((col('display_name').like("%Яндекс%")) | (col('display_name').like("%Yandex%")) |
                      (col('display_name').like("%яндекс%")) | (col('display_name').like("%yandex%"))))
            .withColumn('Timestamp', F.current_timestamp().cast(T.IntegerType()))
            .select(
                (company_size_udf(col('size'))).alias('Description'),
                F.coalesce(phone_decorator_udf(col('secure_phone_number')), lit('-')).alias('Phone_1'),
                add_brackets(col('email')).alias('Email'),
                F.lit("trial").alias('Lead_Source'),
                F.lit('Tracker_scoring').alias('Lead_Source_Description'),
                col('firstname').alias('First_name'),
                col('lastname').alias('Last_name'),
                col('display_name').alias('Account_name'),
                col('Timestamp')
            )
        )


        existing_leads = (
            spark.read.yt(dm_crm_leads_path)
            .select(phone_decorator_udf(col('phone_mobile').cast(T.StringType())).alias('Phone_1'))
        )


        crm_known_leads = (
            spark.read.yt(leads_path)
            .select(phone_decorator_udf(col('Phone_1')).alias('Phone_1'))
        )

        final_leads = (
            naming
            .join(existing_leads, on='Phone_1', how='leftanti')
            .join(crm_known_leads, on='Phone_1', how='leftanti')
            .select(col('*'))
            .distinct()
            .limit(50)
        )

        final_leads.coalesce(1).write.yt(result_path, mode='append')


if __name__ == '__main__':
    main()
