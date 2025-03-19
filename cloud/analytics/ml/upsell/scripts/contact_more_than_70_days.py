# pylint: disable=no-value-for-parameter
import os
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"
import pip  # needed to use the pip functions
pip.main(['freeze'])

import click
import logging.config
import spyt
from spyt import spark_session
from clan_tools.utils.spark import SPARK_CONF_SMALL
from clan_tools.utils.spark import safe_append_spark
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from upsell.spyt.upsell_and_contact_leads import GenerateUpsellLeads

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--result_path', default="//home/cloud_analytics/export/crm/update_call_center_leads/update_leads_test")
@click.option('--history_path', default="//home/cloud_analytics/ml/upsell/upsell_and_contact_more_than_70_days/upsell_and_contact_history_test")
def generate_leads(result_path: str, history_path: str):

    logger.info('Starting source project')
    yt_token = os.environ["SPARK_SECRET"] if "SPARK_SECRET" in os.environ else os.environ["YT_TOKEN"]
    yt_adapter = YTAdapter(yt_token)

    # spark session
    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_SMALL) as spark:
        spyt.info(spark)
        leads_generator = GenerateUpsellLeads(spark)
        upsell_leads, contact_more_than_70_days = leads_generator.generate_upsell_and_contact_leads()

        result_leads = upsell_leads.union(contact_more_than_70_days).coalesce(1)
        safe_append_spark(yt_adapter=yt_adapter, spdf=result_leads, path=result_path, mins_to_wait=60)
        safe_append_spark(yt_adapter=yt_adapter, spdf=result_leads, path=history_path, mins_to_wait=60)

    yt_adapter.optimize_chunk_number(result_path)
    yt_adapter.optimize_chunk_number(history_path)

if __name__ == '__main__':
    generate_leads()
