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
import pyspark.sql.functions as F
from pyspark.sql.functions import col
from datetime import datetime
from clan_tools.utils.spark import SPARK_CONF_SMALL
from clan_tools.utils.spark import safe_append_spark
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from leads_flow.onboarding_leads import LeadsFlow

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--is_prod', is_flag=True, default=False)
def main(is_prod: bool):
    logger.info('Starting build sankey diagram...')
    yt_token = os.environ["SPARK_SECRET"] if "SPARK_SECRET" in os.environ else os.environ["YT_TOKEN"]
    yt_adapter = YTAdapter(yt_token)

    prod_postfix = '' if is_prod else '_test'
    result_table = f'//home/cloud_analytics/ml/leads_flow/graph_sankey{prod_postfix}'
    result_table_name = result_table.split('/')[-1]
    result_table_folder = '/'.join(result_table.split('/')[:-1])
    rep_date = datetime.now().strftime('%Y-%m-%d')

    # spark session
    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_SMALL) as spark:
        spyt.info(spark)

        if result_table_name in yt_adapter.yt.list(result_table_folder):
            max_available_date = spark.read.yt(result_table).agg(F.max('calc_date')).collect()[0][0] or ''
            if max_available_date >= rep_date:
                spark.read.yt(result_table).filter(col('calc_date') < rep_date).write.yt(result_table, mode='overwrite')

        leads_flow = LeadsFlow(spark, yt_adapter)
        spdf_graph_sankey = leads_flow.get_sankey_graph()
        safe_append_spark(yt_adapter, spdf_graph_sankey, result_table)

    yt_adapter.optimize_chunk_number(result_table)


if __name__ == '__main__':
    main()
