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


@click.command('cloud_graph')
@click.option('--cloud_criterions_path')
@click.option('--cloud_crm_accounts_path')
@click.option('--result_edges')
@click.option('--result_nodes')
def cloud_graph(cloud_criterions_path: str, cloud_crm_accounts_path:str, result_edges: str, result_nodes: str):
    with spark_session(yt_proxy="hahn", spark_conf_args=DEFAULT_SPARK_CONF, driver_memory='2G') as spark:
        cloud_criterions_all = (
            spark.read
            .yt(cloud_criterions_path)
            .filter((~col('value').contains('fake')))
        )

        cloud_crm_accounts = spark.read.yt(cloud_crm_accounts_path)

        cloud_criterions = cloud_criterions_all.filter(
            col('criterion') != 'name').cache()

        cloud_passport_ids = (
            cloud_criterions
            .select(F.concat(
                lit('passport_id'), lit('::'), col('passport_id'))
                .alias('id'),
                lit('passport_id').alias('type'),
                col('passport_id').alias('value'),
                F.array(lit('cloud')).alias('sources')
            )
            .distinct()
        )

        cloud_crm_accounts_edges = (
            cloud_crm_accounts
            .select(F.concat(
                lit('billing_account_id'), lit('::'), col('billing_account_id'))
                .alias('src'),
               F.concat(
                lit('inn'), lit('::'), col('inn'))
            )
            .distinct()
        )

        cloud_other_ids = (
            cloud_criterions
            .select(F.concat(
                col('criterion'), lit('::'), col('value'))
                .alias('id'),
                col('criterion').alias('type'),
                col('value'),
                F.array(lit('cloud')).alias('sources')
            )
            .distinct()
        )

        cloud_nodes = (
            cloud_passport_ids
            .unionAll(cloud_other_ids)
        ).distinct()

        passport_edges = (
            cloud_criterions
            .select(
                F.concat(
                    lit('passport_id'), lit('::'), col('passport_id'))
                .alias('src'),
                F.concat(
                    col('criterion'), lit('::'), col('value'))
                .alias('dst')
            )
            .distinct()
        ).filter(col('criterion') != 'name')

        cloud_edges = (
            passport_edges
            .unionAll(cloud_crm_accounts_edges)
        )

        cloud_edges.write.yt(result_edges, mode='overwrite')
        cloud_nodes.write.yt(result_nodes, mode='overwrite')


if __name__ == '__main__':
    cloud_graph()
