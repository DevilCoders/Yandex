
import logging.config
import os

import click
import pyspark.sql.dataframe as spd
import pyspark.sql.functions as F
import pyspark.sql.types as T
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.spark import DEFAULT_SPARK_CONF
from pyspark.sql.functions import col, lit
from spyt import spark_session

os.environ["JAVA_HOME"] = "/usr/local/jdk-11"


logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


@click.command("cloud_graph")
def cloud_graph():
    with spark_session(yt_proxy="hahn", spark_conf_args=DEFAULT_SPARK_CONF, driver_memory='2G') as spark:
        onboarding_leads = (
            spark
            .read
            .schema_hint({'passport_ids': T.ArrayType(T.StringType())})
            .yt('//home/cloud_analytics/tmp/onboarding_leads')
        ).orderBy('company_size_revenue', ascending=False).cache()

        for i in range(10):
            print('-----------------------')
            print(onboarding_leads.show(20) )


if __name__ == '__main__':
    cloud_graph()
