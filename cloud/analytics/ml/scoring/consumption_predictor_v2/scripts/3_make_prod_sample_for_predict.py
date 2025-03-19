# pylint: disable=no-value-for-parameter
import os
print(os.listdir('.'))
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"
import pip  # needed to use the pip functions
pip.main(['freeze'])

import click
from spyt import spark_session
import logging.config
import pandas as pd
from datetime import timedelta
import spyt
import pyspark.sql.functions as F
from pyspark.sql.functions import col, lit
from clan_tools.utils.spark import SPARK_CONF_LARGE
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from consumption_predictor_v2.train_config.prod_features import prod_model_colnames
from consumption_predictor_v2.utils.helpers_spark import make_pool

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--results_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/prod_results")
@click.option('--features_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/actual_features")
@click.option('--temporary_calcs_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/model/prod/last_predict_samples")
def make_prod_sample(results_path, features_path, temporary_calcs_path):
    logger.info("Start collecting Pool for prediction")
    yt_token = os.environ["SPARK_SECRET"] if ("SPARK_SECRET" in os.environ) else os.environ["YT_TOKEN"]
    yt_adapter = YTAdapter(yt_token)

    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_LARGE) as spark:
        spyt.info(spark)

        features_dts = (
            spark.read.yt(features_path)
            .withColumn("next_14d_cons", lit("NULL"))
            .select(*prod_model_colnames)
        )

        results_table_name = results_path.split('/')[-1]
        results_folder_name = '/'.join(results_path.split('/')[:-1])
        if results_table_name in yt_adapter.yt.list(results_folder_name):
            date_from = spark.read.yt(results_path).agg(F.max("billing_record_msk_date")).collect()[0][0]
            date_from = (pd.to_datetime(date_from)+timedelta(days=1)).strftime('%Y-%m-%d')
        else:
            date_from = '2021-08-01'
        date_to = features_dts.agg(F.max("billing_record_msk_date")).collect()[0][0]
        logger.info(f"Prediction period: {date_from} to {date_to}")

        features_dts_truncated = (
            features_dts.filter(col("billing_record_msk_date")>=date_from).filter(col("billing_record_msk_date")<=date_to).cache()
            .withColumn("SampleId", F.concat(col("billing_account_id"), lit("_"), col("billing_record_msk_date")))
        )
        ids_dts = features_dts_truncated.select("billing_account_id", "billing_record_msk_date", "SampleId")
        ids_dts.write.yt(f'{temporary_calcs_path}/ids', mode='overwrite')
        predict_pool, pool_cd = make_pool(spark, features_dts_truncated)
        logger.info("Start calculating dataset & saving dataset...")
        predict_pool.write.yt(f'{temporary_calcs_path}/predict_pool', mode='overwrite')
        pool_cd.write.yt(f'{temporary_calcs_path}/pool.cd', mode='overwrite')

    yt_adapter.optimize_chunk_number(f'{temporary_calcs_path}/predict_pool')
    yt_adapter.optimize_chunk_number(f'{temporary_calcs_path}/ids')


if __name__ == '__main__':
    make_prod_sample()
