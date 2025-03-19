# pylint: disable=no-value-for-parameter
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
import click
from clan_tools.utils.spark import DEFAULT_SPARK_CONF
from consumption_predictor.data_adapters.crm.CRMConsThresholdAdapter import CRMConsThresholdAdapter
from spyt import spark_session

import logging.config
import os

from clan_tools.data_adapters.crm.CRMModelAdapter import upsale_to_update_leads
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"


logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


@click.command('save_to_crm')
@click.option('--crm_path')
def save_to_crm(crm_path: str):

    with spark_session(yt_proxy="hahn", spark_conf_args=DEFAULT_SPARK_CONF, driver_memory='2G') as spark:

        filtered_restore = CRMConsThresholdAdapter(spark,
                                                   YTAdapter(
                                                       token=os.environ["SPARK_SECRET"]),
                                                   crm_table_to=crm_path,
                                                   consumption_threshold=150000,
                                                   days_period=90).restore_recycled()
        upsale_to_update_leads(filtered_restore).write.yt(
            crm_path, mode='append')


if __name__ == '__main__':
    save_to_crm()
