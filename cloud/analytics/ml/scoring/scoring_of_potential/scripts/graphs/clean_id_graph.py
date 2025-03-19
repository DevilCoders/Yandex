import logging.config
import os
from graphframes import GraphFrame
import click
import pyspark.sql.dataframe as spd
import pyspark.sql.functions as F
import pyspark.sql.types as T
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.spark import SPARK_CONF_GRAPHS
from pyspark.sql.functions import col, last, lit
from spyt import spark_session
import pandas as pd
from pyspark.sql.functions import pandas_udf
from pyspark.sql.session import SparkSession

os.environ["JAVA_HOME"] = "/usr/local/jdk-11"


logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)



@click.command('clean_id_graph')
@click.option('--input_edges')
@click.option('--input_nodes')
@click.option('--result_edges')
@click.option('--result_nodes')
@click.option('--checkpoint_dir')
def clean_id_graph(result_edges: str, result_nodes: str, input_edges: str, input_nodes: str, 
                checkpoint_dir: str):
    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_GRAPHS, driver_memory='2G') as spark:
        nodes = (spark
                 .read
                 .schema_hint({'sources': T.ArrayType(T.StringType()),
                               'events':  T.ArrayType(T.StringType())})
                 .yt(input_nodes)).cache()

        edges = spark.read.yt(input_edges).cache()

        g = GraphFrame(nodes, edges).cache()


        def quantile(n, name):
            return F.expr(f'percentile_approx(degree, {n})').alias(f'quantile_{name}')

        quantiles = (
            g.vertices
            .groupby('type')
            .agg(quantile(0.99, 'bad'))
        ).cache()

        g_quantiles = GraphFrame(
            g.vertices
                .join(quantiles, on='type')
                .drop('component'), 
            edges
        )

        bad_connections = ['email', 'phone']
        filtered_g = (
            g_quantiles
            .filterVertices(
                ((col('degree') <= 100) & (col('type') == 'email'))
                |  ((col('degree') <= 200) & (col('type') == 'phone'))
                | ~col('type').isin(bad_connections)
            )
            .dropIsolatedVertices()
        )

        final_g = GraphFrame(nodes.select('id', 'type'), filtered_g.edges).cache()
        spark.sparkContext.setCheckpointDir(f'yt:/{checkpoint_dir}')
        connected_components = final_g.connectedComponents().cache()
      

        nodes_with_info = (
            (nodes
            .drop('degree').drop('component'))
            .join(final_g.degrees, on='id', how='left')
            .join(connected_components.select('id', 'type', 'component'), on=['id', 'type'], how='left')
            .withColumn('name', F.coalesce(col('cloud_account_name'), col('spark_name')))
            .withColumn('degree', F.coalesce(col('degree'), lit(0)))

        ).cache()



        final_g.edges.write.yt(result_edges, mode='overwrite')
        nodes_with_info.write.yt(result_nodes, mode='overwrite')


if __name__ == '__main__':
    clean_id_graph()
