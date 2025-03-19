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
from datetime import datetime, timedelta
import spyt
import pyspark.sql.functions as F
from pyspark.sql.functions import col
from clan_tools.utils.spark import SPARK_CONF_MEDIUM
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from consumption_predictor_v2.utils.helpers_spark import next_N_days_cons

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


# main function
@click.command()
@click.option('--features_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/actual_features")
@click.option('--target_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/actual_target")
@click.option('--rebuild', is_flag=True, default=False)
def collect_target(features_path, target_path, rebuild):
    yt_token = os.environ["SPARK_SECRET"] if ("SPARK_SECRET" in os.environ) else os.environ["YT_TOKEN"]
    yt_adapter = YTAdapter(yt_token)
    target_folder_path = '/'.join(target_path.split('/')[:-1])
    target_table_name = target_path.split('/')[-1]

    if rebuild and (target_table_name in yt_adapter.yt.list(target_folder_path)):
        copy_date = datetime.now().strftime('%Y-%m-%d')
        old_target_path = f'//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/old_features/target_as_of_{copy_date}'
        yt_adapter.yt.copy(target_path, old_target_path, force=True)
        yt_adapter.yt.remove(target_path)

    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_MEDIUM) as spark:
        spyt.info(spark)

        if target_table_name in yt_adapter.yt.list(target_folder_path):
            period_start = spark.read.yt(target_path).agg(F.max("billing_record_msk_date")).collect()[0][0]
            period_start = (pd.to_datetime(period_start)+timedelta(days=1)).strftime('%Y-%m-%d')
        else:
            period_start = spark.read.yt(features_path).agg(F.min("billing_record_msk_date")).collect()[0][0]

        features_dts = (
            spark.read.yt(features_path)
            .select(
                "billing_account_id",
                F.to_date("billing_record_msk_date").alias("billing_record_msk_date"),
                "billing_record_total_rub"
            )
            .filter(col("billing_record_msk_date")>=period_start)
        )

        target_dts = (
            features_dts
            .select(
                "billing_account_id",
                "billing_record_msk_date",
                next_N_days_cons(14)
            )
            .filter(~col("next_14d_cons").isNull())
            .withColumn("billing_record_msk_date", F.date_format(col("billing_record_msk_date"), "yyyy-MM-dd"))
        )

        logger.info('Appending a part to YT table')
        target_dts.write.optimize_for("scan").yt(target_path, mode='append')

    yt_adapter.optimize_chunk_number(target_path)


if __name__ == '__main__':
    collect_target()
