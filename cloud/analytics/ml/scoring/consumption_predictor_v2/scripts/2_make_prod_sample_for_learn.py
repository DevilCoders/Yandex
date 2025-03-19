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
from clan_tools.utils.spark import SPARK_CONF_MEDIUM
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from consumption_predictor_v2.utils.helpers_spark import max_by
from consumption_predictor_v2.train_config.prod_features import prod_model_colnames
from consumption_predictor_v2.utils.helpers_spark import make_pool

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


# main function
@click.command()
@click.option('--features_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/actual_features")
@click.option('--target_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/actual_target")
@click.option('--samples_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/model/prod/last_predict_samples")
def make_prod_datasets(features_path, target_path, samples_path):
    logger.info("Start collecting Pool for training")
    yt_token = os.environ["SPARK_SECRET"] if ("SPARK_SECRET" in os.environ) else os.environ["YT_TOKEN"]
    yt_adapter = YTAdapter(yt_token)

    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_MEDIUM) as spark:
        spyt.info(spark)

        target_spdf = spark.read.yt(target_path)
        period_end = target_spdf.agg(F.max("billing_record_msk_date")).collect()[0][0]
        period_start = (pd.to_datetime(period_end)-timedelta(days=180)).strftime('%Y-%m-%d')

        target_spdf = target_spdf.filter(col("billing_record_msk_date")>=period_start).filter(col("billing_record_msk_date")<=period_end)
        features_spdf = spark.read.yt(features_path).filter(col("billing_record_msk_date")>=period_start).filter(col("billing_record_msk_date")<=period_end)
        general_dts = (
            target_spdf
            .join(features_spdf, on=["billing_account_id", "billing_record_msk_date"])
            .select(prod_model_colnames)
            .withColumn("billing_record_msk_date", F.to_date("billing_record_msk_date"))
            .select(
                F.floor(F.datediff(col("billing_record_msk_date"), lit(period_start))/7).cast("int").alias("br_week_num"),
                F.dayofweek("billing_record_msk_date").alias("br_week_day"),
                *prod_model_colnames
            )
        )

        # take one random observation in a week
        max_br_week_num = general_dts.agg(F.max("br_week_num")).collect()[0][0]
        shuffling_rule = (
            general_dts
            .filter(col("br_week_num")<max_br_week_num)
            .select(
                "billing_account_id",
                "br_week_num",
                "br_week_day",
                F.rand(seed=42).alias("random_col")
            )
            .groupby("billing_account_id", "br_week_num")
            .agg(
                max_by("br_week_day", "random_col").alias("br_week_day")
            )
        )
        filtered_gen_dts = (
            shuffling_rule
            .join(general_dts, on=["billing_account_id", "br_week_num", "br_week_day"], how="left")
            .cache()
        )

        def spark_train_test_oot_split(spark_df, random_state=42, ba_split_ratio=0.7, time_split_ratio=0.7):
            # split billing_accounts randomly and weeks in time
            all_bas = (
                spark_df
                .select("billing_account_id")
                .distinct()
                .select(
                    "billing_account_id",
                    F.rand(seed=random_state).alias("random_col")
                )
            )
            tr_bas = all_bas.filter(col("random_col")<=ba_split_ratio)
            ntr_bas = all_bas.filter(col("random_col")>ba_split_ratio)
            oot_border = int(max_br_week_num*time_split_ratio)

            # make datasets
            train_base = filtered_gen_dts.join(tr_bas[["billing_account_id"]], on=["billing_account_id"], how="right")
            test_base = filtered_gen_dts.join(ntr_bas[["billing_account_id"]], on=["billing_account_id"], how="right")

            train = train_base.filter(col("br_week_num")<=oot_border)
            test = test_base.filter(col("br_week_num")<=oot_border)
            oot = test_base.filter(col("br_week_num")>oot_border)
            return train, test, oot

        sample_train_val, sample_test, sample_oot = spark_train_test_oot_split(filtered_gen_dts)
        sample_train, _, sample_val = spark_train_test_oot_split(sample_train_val)

        logger.info(f'{samples_path}/train_val')
        sample_train_val = (
            sample_train_val
            .withColumn("billing_record_msk_date", F.date_format(col("billing_record_msk_date"), "yyyy-MM-dd"))
            .select(*prod_model_colnames)
            .withColumn("SampleId", F.concat(col("billing_account_id"), lit("_"), col("billing_record_msk_date")))
        )
        sample_train_val, sample_pool = make_pool(spark, sample_train_val)
        sample_train_val.write.yt(f'{samples_path}/train_val', mode='overwrite')

        logger.info(f'{samples_path}/train')
        sample_train = (
            sample_train
            .withColumn("billing_record_msk_date", F.date_format(col("billing_record_msk_date"), "yyyy-MM-dd"))
            .select(*prod_model_colnames)
            .withColumn("SampleId", F.concat(col("billing_account_id"), lit("_"), col("billing_record_msk_date")))
        )
        sample_train, _ = make_pool(spark, sample_train)
        sample_train.write.yt(f'{samples_path}/train', mode='overwrite')

        logger.info(f'{samples_path}/val')
        sample_val = (
            sample_val
            .withColumn("billing_record_msk_date", F.date_format(col("billing_record_msk_date"), "yyyy-MM-dd"))
            .select(*prod_model_colnames)
            .withColumn("SampleId", F.concat(col("billing_account_id"), lit("_"), col("billing_record_msk_date")))
        )
        sample_val, _ = make_pool(spark, sample_val)
        sample_val.write.yt(f'{samples_path}/val', mode='overwrite')

        logger.info(f'{samples_path}/test')
        sample_test = (
            sample_test
            .withColumn("billing_record_msk_date", F.date_format(col("billing_record_msk_date"), "yyyy-MM-dd"))
            .select(*prod_model_colnames)
            .withColumn("SampleId", F.concat(col("billing_account_id"), lit("_"), col("billing_record_msk_date")))
        )
        sample_test, _ = make_pool(spark, sample_test)
        sample_test.write.yt(f'{samples_path}/test', mode='overwrite')

        logger.info(f'{samples_path}/oot')
        sample_oot = (
            sample_oot
            .withColumn("billing_record_msk_date", F.date_format(col("billing_record_msk_date"), "yyyy-MM-dd"))
            .select(*prod_model_colnames)
            .withColumn("SampleId", F.concat(col("billing_account_id"), lit("_"), col("billing_record_msk_date")))
        )
        sample_oot, _ = make_pool(spark, sample_oot)
        sample_oot.write.yt(f'{samples_path}/oot', mode='overwrite')

        logger.info(f'{samples_path}/pool.cd')
        sample_pool.write.yt(f'{samples_path}/pool.cd', mode='overwrite')

    yt_adapter.optimize_chunk_number(f'{samples_path}/train_val')
    yt_adapter.optimize_chunk_number(f'{samples_path}/train')
    yt_adapter.optimize_chunk_number(f'{samples_path}/val')
    yt_adapter.optimize_chunk_number(f'{samples_path}/test')
    yt_adapter.optimize_chunk_number(f'{samples_path}/oot')


if __name__ == '__main__':
    make_prod_datasets()
