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


@click.command('cpark_com_dep_cloud_graph')
@click.option('--spark_criterions')
@click.option('--com_dep_cloud_edges')
@click.option('--com_dep_cloud_nodes')
@click.option('--result_edges')
@click.option('--result_nodes')
def spark_com_dep_cloud_graph(
        spark_criterions: str,  
        result_edges: str, 
        result_nodes: str, 
        com_dep_cloud_edges: str, 
        com_dep_cloud_nodes: str):
    with spark_session(yt_proxy="hahn", spark_conf_args=DEFAULT_SPARK_CONF, driver_memory='2G') as spark:
        nodes = (spark
                 .read
                 .schema_hint({'sources': T.ArrayType(T.StringType())})
                 .yt(com_dep_cloud_nodes).cache())

        edges = spark.read.yt(com_dep_cloud_edges).cache()

        spark_criterions = (
            spark.read
            .yt(spark_criterions)
            .filter(~(col('value').contains('fake')))
            .filter(col('criterion') != 'name')
        )

        spark_cloud_criterions = (  
            spark_criterions
            .select(['spark_id', col('criterion').alias('type'), 'value', lit('spark').alias('source')])
            # .join(nodes.select('type', 'value').distinct(), on=['type', 'value'])
        ).cache()


        
        spark_com_dep_cloud_edges = (
            (
                spark_cloud_criterions
                .select(
                    F.concat(
                        lit('spark_id'), lit('::'), col('spark_id'))
                    .alias('src'),
                    F.concat(
                        col('type'), lit('::'), col('value'))
                    .alias('dst')
                )
                .distinct()
            ).unionAll(edges)
        )

        spark_ids = (
            spark_cloud_criterions
            .select(F.concat(
                lit('spark_id'),  lit('::'), col('spark_id'))
                .alias('id'),
                lit('spark_id').alias('type'),
                col('spark_id').alias('value'),
                lit('spark').alias('source')
            )
            .groupby(['id', 'type', 'value'])
            .agg(F.collect_set('source').alias('sources'))
        )

        spark_other_ids =  (
            spark_cloud_criterions
            .select(F.concat(
                        col('type'), lit('::'), col('value'))
                    .alias('id'), 
                    col('type'),
                    col('value'),
                    lit('spark').alias('source')
                )
            .groupby(['id', 'type', 'value'])
            .agg(F.collect_set('source').alias('sources'))
        )

        spark_com_dep_cloud_nodes = (
            (
                nodes
                .unionAll(spark_ids)
                .unionAll(spark_other_ids)
            )
            .groupby(['id', 'type', 'value'])
            .agg(F.array_distinct(F.flatten(F.collect_list('sources'))).alias('sources'))
        )

        spark_com_dep_cloud_edges.write.yt(result_edges, mode='overwrite')
        spark_com_dep_cloud_nodes.write.yt(result_nodes, mode='overwrite')


if __name__ == '__main__':
    spark_com_dep_cloud_graph()
