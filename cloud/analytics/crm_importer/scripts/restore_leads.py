
from pyspark.sql.session import SparkSession
from spyt import spark_session
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.utils.spark import SPARK_CONF_MEDIUM
from clan_tools.logging.logger import default_log_config
import click
import pyspark.sql.dataframe as spd
import logging.config
from crm_importer.restore_leads.columns_mapping import restore_leads2update_leads, dm_crm_leads2restore_leads
import os

os.environ["JAVA_HOME"] = "/usr/local/jdk-11"


logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


def export_leads_table(
        spark: SparkSession,
        input_leads_path: str,
        dm_crm_leads_path: str,
        update_leads_path: str,
        leads_daily_limit: int
):
    input_leads: spd.DataFrame = spark.read.yt(input_leads_path).cache()
    existing_leads: spd.DataFrame = (
        dm_crm_leads2restore_leads(spark.read.yt(dm_crm_leads_path))
        .join(
            input_leads,
            on=['set_lead_source', 'set_lead_source_description'],
            how='leftsemi'
        )
        .cache()
    )
    filtered_leads = (
        input_leads
        .join(existing_leads, on='billing_account_id', how='leftanti')
        .join(existing_leads, on='email', how='leftanti')
        .join(existing_leads, on='phone_mobile', how='leftanti')
        .limit(int(leads_daily_limit))
    )
    prepared_to_update_leads = restore_leads2update_leads(
        filtered_leads).cache()

    update_leads_existing = (
        spark.read.yt(update_leads_path)
        .join(
            prepared_to_update_leads,
            on=['Lead_Source', 'Lead_Source_Description'],
            how='leftsemi'
        )
        .cache()
    )

    filtered_prepared_to_update_leads = (
        prepared_to_update_leads
        .join(update_leads_existing, on='Billing_account_id', how='leftanti')
        .join(update_leads_existing, on='Email', how='leftanti')
        .join(update_leads_existing, on='Phone_1', how='leftanti')
        .limit(int(leads_daily_limit))
    )

    filtered_prepared_to_update_leads.write.yt(
        update_leads_path, mode='append')


@click.command()
@click.option('--input_leads_dir_paths', multiple=True)
@click.option('--dm_crm_leads_path', default='//home/cloud_analytics/dwh/cdm/dm_crm_leads')
@click.option('--update_leads_path',
              default='//home/cloud_analytics/test/export/crm/update_call_center_leads/update_leads')
@click.option('--leads_daily_limit', default=1000)
def main(input_leads_dir_paths: str,  update_leads_path: str, dm_crm_leads_path: str, leads_daily_limit: int = 1000):
    with spark_session(yt_proxy="hahn",
                       spark_conf_args=SPARK_CONF_MEDIUM,
                       driver_memory='2G') as spark:
        yt_adapter = YTAdapter(token=os.environ["SPARK_SECRET"])
        logger.debug(f'Will export leads from {input_leads_dir_paths}')
        for input_leads_dir in input_leads_dir_paths:
            logger.debug(f'Will export leads from {input_leads_dir}')
            tables = yt_adapter.yt.list(input_leads_dir)
            for table in tables:
                input_leads_path = f'{input_leads_dir}/{table}'
                logger.debug(f'Exporting leads from {input_leads_path}')
                export_leads_table(
                    spark,
                    input_leads_path=input_leads_path,
                    dm_crm_leads_path=dm_crm_leads_path,
                    update_leads_path=update_leads_path,
                    leads_daily_limit=leads_daily_limit
                )
                logger.debug(f'Leads from {input_leads_path} are exported')


if __name__ == '__main__':
    main()
