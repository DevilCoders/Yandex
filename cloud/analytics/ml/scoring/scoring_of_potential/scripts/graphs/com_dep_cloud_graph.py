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


@click.command('com_dep_cloud_graph')
@click.option('--spark_to_passport')
@click.option('--com_dep_criterions')
@click.option('--cloud_edges')
@click.option('--cloud_nodes')
@click.option('--result_edges')
@click.option('--result_nodes')
def com_dep_cloud_graph(com_dep_criterions:str, spark_to_passport: str, result_edges: str, result_nodes: str, cloud_edges: str, cloud_nodes: str):
    with spark_session(yt_proxy="hahn", spark_conf_args=DEFAULT_SPARK_CONF, driver_memory='2G') as spark:
        nodes = (spark
                    .read
                    .schema_hint({'sources': T.ArrayType(T.StringType())})
                    .yt(cloud_nodes).cache())

        edges = spark.read.yt(cloud_edges).cache()

        

        com_dep_criterions = (
            spark.read
            .yt(com_dep_criterions)
            .filter(~(col('value').contains('fake')))
            .filter(~(col('criterion').contains('domain')))

        )


        com_dep_spark_passport = (
            spark
            .read
            .schema_hint({'criterion_list': T.ArrayType(T.StringType())})
            .yt(spark_to_passport)
            .select('passport_id', 'spark_id', F.explode('criterion_list').alias('criterion'))
        )

        com_dep_spark_passport_criterions = (
            com_dep_spark_passport
            .join(com_dep_criterions, on=['spark_id', 'criterion'], how='inner')
        ).cache()

        com_dep_criterions_with_source = (
            com_dep_spark_passport_criterions
            .withColumn('criterion_parts', F.split(col('criterion'), '_'))
            .withColumn('criterion_name', col('criterion_parts').getItem(0))
            .select(col('*'), col('criterion')
                            .substr(F.length(col('criterion_name'))+2, 
                                    F.length(col('criterion')))
                    .alias('source')
                    )
            .select('spark_id', 
                    'passport_id',
                    col('criterion_name').alias('type'),
                    'source',
                    'value')
            .distinct()
        ).cache()


        com_dep_cloud_spark_ids = (
            com_dep_criterions_with_source
            .select(F.concat(
                        lit('spark_id'), lit('::'),  col('spark_id'))
                    .alias('id'), 
                    lit('spark_id').alias('type'),
                    col('spark_id').alias('value'),
                    lit('com_dep').alias('source')
                )
            .groupby(['id', 'type', 'value'])
            .agg(F.collect_set('source').alias('sources'))
        )

        com_dep_cloud_passport_ids = (
            com_dep_criterions_with_source
            .select(F.concat(
                        lit('passport_id'), lit('::'), col('passport_id'))
                    .alias('id'), 
                    lit('passport_id').alias('type'),
                    col('passport_id').alias('value'),
                    lit('com_dep').alias('source')
                )
            .groupby(['id', 'type', 'value'])
            .agg(F.collect_set('source').alias('sources'))
        )

        com_dep_cloud_other_ids = (
            com_dep_criterions_with_source
            .select(F.concat(
                        col('type'), lit('::'), col('value'))
                    .alias('id'), 
                    col('type'),
                    col('value'),
                    col('source')
                )
            .groupby(['id', 'type', 'value'])
            .agg(F.collect_set('source').alias('sources'))
        )

        com_dep_cloud_nodes = (
            (
                nodes
                .unionAll(com_dep_cloud_spark_ids)
                .unionAll(com_dep_cloud_passport_ids)
                .unionAll(com_dep_cloud_other_ids)
            )
            .groupby(['id', 'type', 'value'])
            .agg(F.array_distinct(F.flatten(F.collect_list('sources'))).alias('sources'))
        )

        com_dep_cloud_edges = (
        (
            com_dep_criterions_with_source
            .select(
                F.concat(lit('passport_id'), lit('::'),  col('passport_id'))
                .alias('src'),
                F.concat(col('type'), lit('::'), col('value'))
                .alias('dst')
            )
            .distinct()
        ).unionAll(
            com_dep_criterions_with_source
            .select(
                F.concat(
                    lit('spark_id'), lit('::'), col('spark_id'))
                .alias('src'),
                F.concat(
                    col('type'),  lit('::'), col('value'))
                .alias('dst')
            )
            .distinct()
        ).unionAll(edges))

        com_dep_cloud_edges.write.yt(result_edges, mode='overwrite')
        com_dep_cloud_nodes.write.yt(result_nodes, mode='overwrite')


if __name__ == '__main__':
    com_dep_cloud_graph()
