import logging.config
import os

import click
import pyspark.sql.dataframe as spd
import pyspark.sql.functions as F
import pyspark.sql.types as T
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.spark import DEFAULT_SPARK_CONF
from pyspark.sql.functions import  col
from spyt import spark_session
from clan_tools.data_adapters.YTAdapter import YTAdapter
import os



os.environ["JAVA_HOME"] = "/usr/local/jdk-11"


logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


def trim(col, trim_str):
    return F.expr(f"trim(BOTH '{trim_str}' FROM {col})")

@click.command()
@click.option('--crm_accounts')
def crm_industries_stats(crm_accounts: str):
    with spark_session(yt_proxy="hahn", spark_conf_args=DEFAULT_SPARK_CONF, driver_memory='2G') as spark:
        yt_adapter = YTAdapter(token=os.environ['SPARK_SECRET'])
        cloud_crm_acounts = (
            spark.read
                .yt(yt_adapter.last_table_name(crm_accounts))
        ).cache()
        inn_industry = (
            cloud_crm_acounts
        #     .filter(col('industry').isNotNull())
            .filter(col('deleted') == 0)
            .select('inn', 'name', F.split(trim('industry', '^'), '\^,\^').alias('industries'))
            .select('inn', 'name', F.explode('industries').alias('industry'))
        )
        inn_industry_count = (
            inn_industry
                .groupby('industry')
                .agg(F.countDistinct('inn').alias('inns_count'))
                .withColumn('sort_col', -1*col('inns_count'))
                .sort(col('inns_count').desc())
        )

        (
            inn_industry_count.sort('sort_col')
            .write
            .sorted_by('sort_col')
            .mode("overwrite").yt(path)
        )

        

if __name__ == '__main__':
    crm_industries_stats()
