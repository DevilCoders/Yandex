import logging.config
import os
from graphframes import GraphFrame
import click
import pyspark.sql.dataframe as spd
import pyspark.sql.functions as F
import pyspark.sql.types as T
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.spark import SPARK_CONF_GRAPHS
from pyspark.sql.functions import col, lit
from spyt import spark_session
import pandas as pd
from pyspark.sql.functions import pandas_udf
from pyspark.sql.session import SparkSession

os.environ["JAVA_HOME"] = "/usr/local/jdk-11"


logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


def get_spark_node_info(spark: SparkSession, spark_info: spd.DataFrame):
    spark_node_info = (
        spark.read.yt(spark_info)
        .select(F.concat(lit('spark_id'), lit('::'), col('spark_id')).alias('id'),
                col('name').alias('spark_name'),
                lit('spark_id').alias('type'),
                'company_size_revenue',
                'company_size_description',
                'legal_city',
                'workers_range',
                'domain'
                )
    )
    return spark_node_info


def get_cloud_passport_info(spark: SparkSession, cloud_info: spd.DataFrame):
    cloud_node_info = (
        spark.read.yt(cloud_info)
        .withColumn('id', F.concat(lit('passport_id'), lit('::'), col('puid')))
        .groupby('id')
        .agg(F.concat_ws(', ', F.collect_set('account_name')).alias('cloud_account_name'),
             F.collect_set('event').alias('events'))
        .select(col('*'), 
                lit('passport_id').alias('type'),
                lit(True).alias('in_cloud'))
    )

    return cloud_node_info

def get_cloud_billing_info(spark: SparkSession, cloud_info: spd.DataFrame):
    is_last_180_days_cons = (
        (col('event') == 'day_use')
        & (F.to_date('event_time') > F.date_add(F.  current_date(), -180))
    )

    def total_cons(cons_column):
        return F.sum(F.when(is_last_180_days_cons,
                            F.coalesce(cons_column, lit(0)))
                     .otherwise(lit(0))).alias(cons_column)
   

    cloud_billing_info = (
        spark.read.yt(cloud_info)
        .withColumn('id', F.concat(lit('billing_account_id'), lit('::'), col('billing_account_id')))
        .groupby('id')
        .agg(
             total_cons('real_consumption'),
             total_cons('trial_consumption'),
             F.last(col('crm_account_owner_current')).alias('crm_account_owner'),
             F.last(col('crm_account_segment_current')).alias('crm_account_segment_current')
             )
        .select(col('*'), lit('billing_account_id').alias('type'))
    )

    return cloud_billing_info


@click.command('id_graph')
@click.option('--input_edges')
@click.option('--input_nodes')
@click.option('--result_edges')
@click.option('--result_nodes')
@click.option('--spark_info')
@click.option('--cloud_info')
@click.option('--checkpoint_dir')
def id_graph(result_edges: str, result_nodes: str, input_edges: str, input_nodes: str, 
                spark_info: str, cloud_info: str, checkpoint_dir: str):
    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_GRAPHS, driver_memory='2G') as spark:
        nodes = (spark
                 .read
                 .schema_hint({'sources': T.ArrayType(T.StringType())})
                 .yt(input_nodes))

        edges = spark.read.yt(input_edges)

        g = GraphFrame(nodes, edges).cache()

       

        final_g = GraphFrame(g.vertices.join(g.degrees, on='id'), edges)
      

        spark.sparkContext.setCheckpointDir(f'yt:/{checkpoint_dir}')
        connected_components = final_g.connectedComponents().cache()
        spark_node_info = get_spark_node_info(spark, spark_info).cache()
        cloud_passport_info = get_cloud_passport_info(spark, cloud_info).cache()
        cloud_billing_info = get_cloud_billing_info(spark, cloud_info).cache()


        nodes_with_info = (
            final_g.vertices
            .join(connected_components.select('id', 'type', 'component'), on=['id', 'type'], how='left')
            .join(cloud_passport_info, how='left', on=['id', 'type'])
            .join(cloud_billing_info, how='left', on=['id', 'type'])
            .join(spark_node_info, how='left', on=['id', 'type'])
            .withColumn('name', F.coalesce(col('cloud_account_name'), col('spark_name')))
        ).cache()



        final_g.edges.write.yt(result_edges, mode='overwrite')
        nodes_with_info.write.yt(result_nodes, mode='overwrite')


if __name__ == '__main__':
    id_graph()
