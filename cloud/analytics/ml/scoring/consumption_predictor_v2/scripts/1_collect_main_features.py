# pylint: disable=no-value-for-parameter
import os
print(os.listdir('.'))
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"
import pip  # needed to use the pip functions
pip.main(['freeze'])

import click
import logging.config
import pandas as pd
from datetime import datetime, timedelta
import spyt
from spyt import spark_session
import pyspark.sql.functions as F
from pyspark.sql.functions import col, lit
from pyspark.sql.window import Window
from clan_tools.utils.spark import SPARK_CONF_LARGE
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from consumption_predictor_v2.utils.helpers_spark import make_window, next_N_days_cons
from consumption_predictor_v2.feature_collection.spyt_functions import make_grid, make_yc_cons, make_vm_cube

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--features_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/actual_features")
@click.option('--rebuild', is_flag=True, default=False)
def collect_features(features_path, rebuild=False):
    yt_token = os.environ["SPARK_SECRET"] if ("SPARK_SECRET" in os.environ) else os.environ["YT_TOKEN"]
    yt_adapter = YTAdapter(yt_token)
    features_folder_path = '/'.join(features_path.split('/')[:-1])
    features_table_name = features_path.split('/')[-1]

    if rebuild and (features_table_name in yt_adapter.yt.list(features_folder_path)):
        copy_date = datetime.now().strftime('%Y-%m-%d')
        old_features_path = f'//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/old_features/features_as_of_{copy_date}'
        yt_adapter.yt.copy(features_path, old_features_path, force=True)
        yt_adapter.yt.remove(features_path)

    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_LARGE) as spark:
        spyt.info(spark)

        numbers_of_days_to_consider = [1, 7, 14, 15, 21, 30, 45]

        if features_table_name in yt_adapter.yt.list(features_folder_path):
            period_start = spark.read.yt(features_path).agg(F.max("billing_record_msk_date")).collect()[0][0]
            period_start = (pd.to_datetime(period_start)+timedelta(days=1)).strftime('%Y-%m-%d')
        else:
            period_start = '2021-01-01'
        date_yesterday = (datetime.now().date()-timedelta(days=1)).strftime('%Y-%m-%d')
        period_end = min((pd.to_datetime(period_start)+timedelta(days=30)).strftime('%Y-%m-%d'), date_yesterday)

        while (period_start <= period_end):

            logger.info(f'Collecting features in period: {period_start} - {period_end}')

            # make dates grid for filling empty gaps
            grid_start = (pd.to_datetime(period_start)-timedelta(days=max(numbers_of_days_to_consider)+1)).strftime('%Y-%m-%d')
            all_ba_grid = make_grid(spark, grid_start, period_end)

            # yc_consumption features
            yc_cons_full = make_yc_cons(spark, all_ba_grid)

            # vm features
            vm_cube_full = make_vm_cube(spark, all_ba_grid)

            # moving averages
            moving_average = (
                yc_cons_full
                .select(
                    "billing_account_id",
                    F.to_date("billing_record_msk_date").alias("billing_record_msk_date"),
                    "billing_record_cost_rub",
                    "billing_record_total_rub",
                    "billing_record_credit_rub"
                )
                .select(
                    "billing_account_id",
                    "billing_record_msk_date",
                    F.sum(
                        F.when(col("billing_record_cost_rub")==0, lit(1))
                        .otherwise(lit(0))
                    ).over(make_window(Window.unboundedPreceding)).alias("days_not_used"),
                    *[next_N_days_cons(-N) for N in numbers_of_days_to_consider],
                    *[next_N_days_cons(-N, "billing_record_credit_rub", "grants") for N in numbers_of_days_to_consider]
                )
                .withColumn("billing_record_msk_date", F.date_format(col("billing_record_msk_date"), "yyyy-MM-dd"))
            )

            features_dts = (
                yc_cons_full
                .filter(col("billing_record_msk_date")>=period_start)
                .join(vm_cube_full, on=["billing_account_id", "billing_record_msk_date"])
                .join(moving_average, on=["billing_account_id", "billing_record_msk_date"])
            )

            logger.info('Appending a part to YT table')
            features_dts.write.optimize_for("scan").yt(features_path, mode='append')

            period_start = spark.read.yt(features_path).agg(F.max("billing_record_msk_date")).collect()[0][0]
            period_start = (pd.to_datetime(period_start)+timedelta(days=1)).strftime('%Y-%m-%d')
            period_end = min((pd.to_datetime(period_start)+timedelta(days=30)).strftime('%Y-%m-%d'), date_yesterday)

    yt_adapter.optimize_chunk_number(features_path)


if __name__ == '__main__':
    collect_features()
