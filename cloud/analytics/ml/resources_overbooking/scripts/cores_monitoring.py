# pylint: disable=no-value-for-parameter
import os
print(os.listdir('.'))
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"
import pip  # needed to use the pip functions
pip.main(['freeze'])

# import click
from spyt import spark_session
import logging.config
import pyspark.sql.functions as F
from pyspark.sql.functions import col
import pandas as pd
import spyt
from clan_tools.utils.spark import SPARK_CONF_MEDIUM
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def cores_monitoring(chunk_size: int = 100 * 2 ** 20):
    yt_token = os.environ["SPARK_SECRET"] if ("SPARK_SECRET" in os.environ) else os.environ["YT_TOKEN"]
    yt = YTAdapter(yt_token).yt

    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_MEDIUM) as spark:
        spyt.info(spark)
        result_table = '//home/cloud_analytics/resources_overbooking/resource_monitoring'
        max_dt = (
            spark
            .read
            .yt(result_table)
            .agg(F.max("start_time").alias("start_time"))
            .collect()[0][0]
        )

        def new_part(folder, last_valid_date):
            target_col = folder[folder.rfind('/')+1:]
            from_mon = pd.to_datetime(last_valid_date*1e9).strftime('%Y-%m')
            files_in_folder = [file for file in yt.list(folder) if file>=from_mon]
            files_to_read = [os.path.join(folder, file) for file in files_in_folder]
            df = (
                spark
                .read
                .option("arrow_enabled", "false")
                .yt(*files_to_read)
                .filter(col("start") > last_valid_date)
                .select("node_name", "zone_id", "platform",
                        col("start").alias("start_time"),
                        "metric", target_col)
            )
            return df

        df_last = new_part('//home/cloud_analytics/import/resources/1mon/last', max_dt)
        df_min = new_part('//home/cloud_analytics/import/resources/1mon/min', max_dt)
        df_max = new_part('//home/cloud_analytics/import/resources/1mon/max', max_dt)
        df_avg = new_part('//home/cloud_analytics/import/resources/1mon/avg', max_dt)
        df_sum = new_part('//home/cloud_analytics/import/resources/1mon/sum', max_dt)

        join_cols = ["node_name", "zone_id", "platform", "start_time", "metric"]
        results = (
            df_last
            .join(df_min, on=join_cols, how='full')
            .join(df_max, on=join_cols, how='full')
            .join(df_avg, on=join_cols, how='full')
            .join(df_sum, on=join_cols, how='full')
        )

        results.write.optimize_for("scan").yt(result_table, mode='append')
        yt.transform(result_table, desired_chunk_size=chunk_size)

if __name__ == "__main__":
    cores_monitoring()
