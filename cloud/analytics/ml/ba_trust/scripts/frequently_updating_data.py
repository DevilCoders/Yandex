import logging.config
import os
import pyspark.sql.functions as F
import pyspark.sql.types as T
from pyspark.sql.window import Window
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.spark import SPARK_CONF_SMALL
from pyspark.sql.functions import col
from spyt import spark_session
# if you want to run local use spark-submit or something


os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"


logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def main():
    result = '//home/cloud_analytics/ml/ba_trust/frequent_data'
    ba_balance = '//home/cloud_analytics/ml/ba_trust/billing_accounts_balance'
    dm_ba_crm_tags = '//home/cloud-dwh/data/prod/cdm/dm_ba_crm_tags'

    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_SMALL, driver_memory='2G') as spark:

        ba_cross_dates = (
            spark.read.yt(dm_ba_crm_tags)
            .filter(col('date') >= '2018-01-01')
            .withColumn('unix_date', F.unix_timestamp(F.to_date(col('date'))))
            .select(col('date').alias('msk_date'), col('date').cast(T.StringType()).alias('date'), 'unix_date', 'billing_account_id').distinct()
        )

        balance = (
            spark.read.yt(ba_balance)
            .select(
                'billing_account_id',
                'date',
                'balance'
            )
            .distinct()
        )

        window = Window.partitionBy('billing_account_id').orderBy('date').rowsBetween(Window.unboundedPreceding, 0)
        data = (
            ba_cross_dates
            .join(balance, on=['billing_account_id', 'date'], how='left')
            .withColumn('balance_float', F.last(col('balance'), ignorenulls=True).over(window)).fillna(0, ['balance_float'])
        )

        data.coalesce(1).write.yt(result, mode='overwrite')


if __name__ == '__main__':
    main()
