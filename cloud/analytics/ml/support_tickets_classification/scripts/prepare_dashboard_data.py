import logging.config
import os
import click
import pyspark.sql.functions as F
import pyspark.sql.types as T
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.spark import SPARK_CONF_SMALL
from pyspark.sql.functions import col
from spyt import spark_session
# if you want to run local use spark-submit or something


os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"

logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


@click.command()
@click.option('--support_issues_path', default="//home/startrek/tables/prod/yandex-team/queue/CLOUDSUPPORT/issues")
@click.option('--components_path', default="//home/startrek/tables/prod/yandex-team/queue/CLOUDSUPPORT/components")
@click.option('--white_list_path', default="//home/cloud_analytics/ml/support_tickets_classification/components_white_list")
@click.option('--result_path', default="//home/cloud_analytics/ml/support_tickets_classification/model_metrics_data")
def main(support_issues_path: str, components_path: str, white_list_path: str, result_path: str):
    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_SMALL, driver_memory='2G') as spark:
        components_white_list = spark.read.yt(white_list_path).select('component_names').rdd.map(lambda row : row[0]).collect()
        dt = '2021-07-19'

        issues = (
            spark.read
            .schema_hint({
                'components': T.ArrayType(T.StringType()),
                'tags': T.ArrayType(T.StringType()),
                'customFields': {'additionalTags': T.StringType()}
                })
            .yt(support_issues_path)
            .select(
                'key', 'tags', F.array_contains(col('tags'), 'ml-reply').alias('verified'),
                F.from_unixtime(col('created').cast(T.LongType())/F.lit(1000)).alias('creation_date'),
                'customFields', F.explode_outer('components').alias('components'))
        )

        components = (
            spark.read.yt(components_path)
            .select('id',
                    col('name').alias('component_name'))
        )

        tickets_flat = (
            issues
            .join(components, on=issues.components == components.id, how='left')
        )

        tickets_with_components = (
            tickets_flat
            .withColumn('clean_component_name', F.when(~col('component_name').isin(components_white_list) , None).otherwise(col('component_name')))
            .groupBy('key', 'creation_date')
            .agg(
                F.first('customFields').alias('customFields'),
                F.first('verified').alias('verified'),
                F.first('tags').alias('tags'),
                F.collect_set('clean_component_name').alias('real_components')
            )
            .withColumn('additionalTags', col('customFields.additionalTags'))
            .filter(F.to_date(col('creation_date')).cast(T.StringType()) >= dt)
            .replace("", None, 'additionalTags')
            .replace("-", None, 'additionalTags')
            .withColumn('additionalTagsSplit', F.split(col('additionalTags'), ", "))
            .withColumn('predicted_components', F.coalesce(col('additionalTagsSplit'), F.array()))
            .select('key', 'creation_date', 'real_components', 'predicted_components', 'verified')

        )

        tickets_with_components.coalesce(1).write.yt(result_path, mode='overwrite')


if __name__ == '__main__':
    main()
