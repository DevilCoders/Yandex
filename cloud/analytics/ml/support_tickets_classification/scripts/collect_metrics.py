import logging.config
import os
import click
import pyspark.sql.functions as F
import pyspark.sql.types as T
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.spark import SPARK_CONF_MEDIUM
from pyspark.sql.functions import col
from spyt import spark_session
# if you want to run local use spark-submit or something


os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"

logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


@click.command()
@click.option('--model_metrics_data', default="//home/cloud_analytics/ml/support_tickets_classification/model_metrics_data")
@click.option('--white_list_path', default="//home/cloud_analytics/ml/support_tickets_classification/components_white_list")
@click.option('--result_path', default="//home/cloud_analytics/ml/support_tickets_classification/model_metrics")
@click.option('--extra', default=False)
def main(white_list_path: str, model_metrics_data: str, result_path: str, extra: bool):
    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_MEDIUM, driver_memory='2G') as spark:
        components_white_list = spark.read.yt(white_list_path).select('component_names').rdd.map(lambda row : row[0]).collect()

        metrics = (
            spark.read
            .schema_hint({
                'predicted_components': T.ArrayType(T.StringType()),
                'real_components': T.ArrayType(T.StringType())
            })
            .yt(model_metrics_data)
            .withColumn('classified_by_model', F.size(col('predicted_components')) != 0)
            .withColumn('classified_by_support', F.size(col('real_components')) != 0)
            .withColumn('TP', F.size(F.array_intersect(col('real_components'), col('predicted_components'))))
            .withColumn('FN', F.size(F.array_except(col('real_components'), col('predicted_components'))))
            .withColumn('FP', F.size(F.array_except(col('predicted_components'), col('real_components'))))
            .withColumn('U', F.size(F.array_union(col('predicted_components'), col('real_components'))))
            .withColumn('Accuracy', col('TP')/col('U'))
            .withColumn('Precision', col('TP')/(col('TP') + col('FP')))
            .withColumn('Recall', col('TP')/(col('TP') + col('FN')))
            .withColumn('hasAll', (col('TP') == F.size(col('predicted_components'))).cast(T.IntegerType()))
            .withColumn('containsAll', (col('TP') == F.size(col('real_components'))).cast(T.IntegerType()))
            .withColumn('hasAny', (col('TP') >= 1).cast(T.IntegerType()))
            .withColumn('equal', (col('TP') == col('U')).cast(T.IntegerType()))
        )

        metrics.drop('additionalTags', 'additionalTagsSplit', 'TP', 'FN', 'FP', 'U').coalesce(1).write.yt(result_path, mode='overwrite')

        if extra:
            extra_metrics = metrics.select('key', 'creation_date', 'real_components', 'predicted_components')
            lack_metrics = metrics.select('key', 'creation_date', 'real_components', 'predicted_components')

            for component in components_white_list:
                extra_metrics = (
                    extra_metrics
                    .withColumn(
                        component,
                        F.array_contains(F.array_except(col('real_components'), col('predicted_components')), component).cast('int'))
                )
                lack_metrics = (
                    lack_metrics
                    .withColumn(
                        'temp',
                        F.array_contains(F.array_except(col('predicted_components'), col('real_components')), component)
                    )
                    .fillna(False, 'temp')
                    .withColumn(component, col('temp').cast('int'))
                )

            extra_metrics.coalesce(1).write.yt(result_path + '_extra', mode='overwrite')
            lack_metrics.drop('temp').coalesce(1).write.yt(result_path + '_lack', mode='overwrite')


if __name__ == '__main__':
    main()
