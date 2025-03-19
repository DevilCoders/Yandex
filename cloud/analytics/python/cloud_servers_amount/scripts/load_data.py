# pylint: disable=no-value-for-parameter
import os
print(os.listdir('.'))
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"
import pip  # needed to use the pip functions
pip.main(['freeze'])

import click
from spyt import spark_session
import logging.config
import pandas as pd
from datetime import datetime
import spyt
from clan_tools.utils.spark import SPARK_CONF_SMALL
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from cloud_servers_amount.bot_client import BotClient

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--result_table_path', default="//home/cloud_analytics/import/cloud_servers_info/resources_info")
def load_data(result_table_path):
    logger.info("Clear backups")
    N = 60
    backups_folder = '/'.join(result_table_path.split('/')[:-1]) + '/backups'
    yt_token = os.environ["SPARK_SECRET"] if ("SPARK_SECRET" in os.environ) else os.environ["YT_TOKEN"]
    yt_adapter = YTAdapter(yt_token)
    yt_adapter.leave_last_N_tables(backups_folder, N-1)

    bot_client = BotClient()
    bot_client.get_hosts_by_filter()
    res = bot_client.get_servers_info_by_filter()
    df_res = pd.DataFrame(res)
    df_res['date_modified'] = datetime.now().strftime('%Y-%m-%d')

    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_SMALL) as spark:
        spyt.info(spark)
        logger.info(f'Rebuild results to {result_table_path}')
        spdf_res_prev = spark.read.yt(result_table_path)
        spdf_res = spark.createDataFrame(df_res).select(spdf_res_prev.columns).union(spdf_res_prev).distinct()
        spdf_res.write.yt(result_table_path, mode='overwrite')


if __name__ == '__main__':
    load_data()
