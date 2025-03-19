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
import spyt
from pyspark.sql.functions import col
from clan_tools.utils.spark import SPARK_CONF_MEDIUM
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--results_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/actual_results")
@click.option('--temporary_calcs_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/model/prod/last_predict_samples")
def save_prod_forecast(results_path, temporary_calcs_path):
    logger.info("Start Saving process")
    yt_token = os.environ["SPARK_SECRET"] if ("SPARK_SECRET" in os.environ) else os.environ["YT_TOKEN"]
    yt_adapter = YTAdapter(yt_token)

    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_MEDIUM) as spark:
        spyt.info(spark)

        ids_dts = spark.read.yt(f'{temporary_calcs_path}/ids')
        res_dts = spark.read.yt(f'{temporary_calcs_path}/result')
        final_res_dts = (
            ids_dts.join(res_dts, on="SampleId", how="left")
            .select("billing_account_id", "billing_record_msk_date", col("RawFormulaVal").alias("next_14d_cons_pred"))
        )
        final_res_dts.write.yt(results_path, mode='append')

    yt_adapter.optimize_chunk_number(results_path)


if __name__ == '__main__':
    save_prod_forecast()
