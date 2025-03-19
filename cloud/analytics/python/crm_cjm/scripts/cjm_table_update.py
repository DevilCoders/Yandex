# pylint: disable=no-value-for-parameter
import os
import click
import logging.config
from spyt import spark_session
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.utils.spark import SPARK_CONF_SMALL
from crm_cjm.spyt import CjmTable


os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--is_prod', is_flag=True, default=False)
def generate_leads(is_prod: bool) -> None:

    logger.info('Starting source project')

    postfix = '' if is_prod else '_test'
    cjm_dash_result = '//home/cloud_analytics/data_swamp/projects/crm_cjm/dash_table' + postfix
    crm_leads_result = '//home/cloud_analytics/data_swamp/projects/crm_cjm/leads' + postfix

    yt_token = os.environ["SPARK_SECRET"] if "SPARK_SECRET" in os.environ else os.environ["YT_TOKEN"]
    yt_adapter = YTAdapter(yt_token)

    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_SMALL, driver_memory='2G') as spark:

        crm_cjm = CjmTable(spark, yt_adapter)
        spdf_crm_leads = crm_cjm.get_crm_historical_data()
        spdf_crm_cjm = crm_cjm.get_crm_cjm()

        if is_prod:
            spdf_crm_leads.write.mode('overwrite').yt(crm_leads_result)
            spdf_crm_cjm.write.mode('overwrite').yt(cjm_dash_result)
        else:
            spdf_crm_leads.limit(5).write.mode('overwrite').yt(crm_leads_result)
            spdf_crm_cjm.limit(5).write.mode('overwrite').yt(cjm_dash_result)

    yt_adapter.optimize_chunk_number(cjm_dash_result)
    yt_adapter.optimize_chunk_number(crm_leads_result)


if __name__ == '__main__':
    generate_leads()
