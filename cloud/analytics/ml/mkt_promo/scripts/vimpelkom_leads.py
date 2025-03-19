# pylint: disable=no-value-for-parameter
import os
print(os.listdir('.'))
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"
import pip  # needed to use the pip functions
pip.main(['freeze'])

import click
import numpy as np
import pandas as pd
import logging.config
import spyt
from spyt import spark_session
from datetime import datetime, timedelta
import pyspark.sql.functions as F
from pyspark.sql.window import Window
from pyspark.sql.functions import col, lit
from clan_tools.utils.spark import SPARK_CONF_SMALL
from clan_tools.utils.spark import safe_append_spark
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.data_adapters.crm.CRMHistoricalDataAdapter import CRMHistoricalDataAdapter
from mkt_promo.promocodes import GetPromocodes

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def days_ago_unix(num_days: int) -> str:
    """Builds date with `num_days` lag in past and returns it in unix-formatted text
    """
    date_format = '%Y-%m-%d'
    dt = datetime.now() - timedelta(num_days)
    dt_unix = dt.strftime(date_format)
    return dt_unix


@click.command()
@click.option('--is_prod', is_flag=True, default=False)
def make_mkt_promo_leads(is_prod):
    yt_token = os.environ["SPARK_SECRET"] if ("SPARK_SECRET" in os.environ) else os.environ["YT_TOKEN"]
    yt_adapter = YTAdapter(yt_token)

    postfix = "" if is_prod else "_test"
    path_to_queue = '//home/cloud_analytics/ml/mkt_promo/assign_queue'
    path_to_tickets = '//home/cloud_analytics/ml/mkt_promo/tickets'
    path_to_cons = '//home/cloud-dwh/data/prod/cdm/dm_yc_consumption'
    path_to_leads = '//home/cloud_analytics/export/crm/update_call_center_leads/update_leads' + postfix
    path_to_history = '//home/cloud_analytics/ml/mkt_promo/leads_history' + postfix

    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_SMALL) as spark:
        spyt.info(spark)

        logger.info('Prepare promocodes info...')
        df_queue = spark.read.yt(path_to_queue).sort('num').toPandas()  # staff logins for assigning leads to
        tickets_list = spark.read.yt(path_to_tickets).toPandas()['ticket'].tolist()  # grants tickets list to consider

        # SparkDataFrame with info about BAs consumption and prepaired description for leads
        spdf_cons = (
            spark.read.yt(path_to_cons)
            .filter(col('billing_record_msk_date') <= days_ago_unix(1))
            .filter(col('billing_record_msk_date') >= days_ago_unix(30))
            .groupby('billing_account_id')
            .agg(
                F.round(-F.sum('billing_record_credit_cud_rub') + 1 - 1, 2).alias('grant_30d'),  # `+1-1` used for removing -0.0 values
                F.round(F.sum('billing_record_cost_rub'), 2).alias('real_30d')
            )
            .withColumn('description', F.concat(lit('Consumed grants on '),
                                                col('grant_30d'), lit(' rub, real on '), col('real_30d'), lit(' rub last 30 days')))
        )

        # Leads that are already in CRM (to avoid duplicates)
        mkt_leads = (
            CRMHistoricalDataAdapter(yt_adapter, spark)
            .historical_preds()
            .filter(col('lead_source_crm') == 'mkt Promo')
            .select('billing_account_id')
            .distinct()
        )

        # Collect info about activated promocodes from grants tickets list & their consumption
        win = Window.orderBy(F.coalesce('grant_30d', lit(0)))
        spdf_promo = (
            GetPromocodes(spark)
            .get_promocodes_averall()
            .filter(col('ticket').isin(tickets_list))
            .filter(col('billing_account_id').isNotNull())
            .join(mkt_leads, on='billing_account_id', how='leftanti')
            .join(spdf_cons, on='billing_account_id', how='left')
            .withColumn('description', F.coalesce('description', lit('Consumed grants on 0.0 rub, real on 0.0 rub last 30 days')))
            .withColumn('assign_id', F.row_number().over(win))
        )

        new_leads_count = spdf_promo.count()
        if new_leads_count == 0:
            logger.info('There is no new leads...')

        else:
            logger.info('Robin-round on assigners...')
            N = spdf_promo.count()
            last_login = df_queue['staff_login'][df_queue['last_assigned_to'] == 1].iloc[0]  # last staff login in robin-round algo
            # Preparing queue to make left join to new leads
            queue = df_queue['staff_login'].tolist() * (N // df_queue.shape[0] + 2)
            for i, login in enumerate(queue):
                if login == last_login:
                    break
            queue = queue[i+1:i+N+1]

            spdf_queue = spark.createDataFrame(pd.DataFrame({'assign_id': range(1, N+1), 'staff': queue}))

            # Update staff logins last assigned flag
            if is_prod:
                df_queue_new = df_queue.copy()
                df_queue_new['last_assigned_to'] = np.where(df_queue_new['staff_login'] == queue[-1], 1, 0)
                spdf_queue_new = spark.createDataFrame(df_queue_new).sort('num').coalesce(1)
                spdf_queue_new.write.mode('overwrite').yt(path_to_queue)

            logger.info('Prepare Mkt Promo leads...')
            leads_timestamp = int((datetime.now() + timedelta(hours=3)).timestamp())  # adapting to Moscow timezone (on system is UTC+0)
            res_leads = (
                spdf_promo
                .join(spdf_queue, on='assign_id', how='left')
                .select(
                    lit(leads_timestamp).alias('Timestamp'),
                    lit(None).astype('string').alias('CRM_Lead_ID'),
                    F.concat(lit('["'), "billing_account_id", lit('"]')).alias("Billing_account_id"),
                    lit(None).astype('string').alias('Status'),
                    col('description').alias('Description'),
                    col('staff').alias('Assigned_to'),
                    col('first_name').alias('First_name'),
                    col('last_name').alias('Last_name'),
                    col('phone').alias('Phone_1'),
                    lit(None).astype('string').alias('Phone_2'),
                    F.concat(lit('["'), col('email'), lit('"]')).alias('Email'),
                    lit('mkt promo').alias('Lead_Source'),
                    lit('Leads from Vimpelcom_mass').alias('Lead_Source_Description'),
                    lit(None).astype('string').alias('Callback_date'),
                    lit(None).astype('string').alias('Last_communication_date'),
                    lit(None).astype('string').alias('Promocode'),
                    lit(None).astype('string').alias('Promocode_sum'),
                    lit(None).astype('string').alias('Notes'),
                    lit(None).astype('string').alias('Dimensions'),
                    lit(None).astype('string').alias('Tags'),
                    lit('').alias('Timezone'),
                    col("display_name").alias('Account_name')
                )
                .coalesce(1)
            )

            safe_append_spark(yt_adapter=yt_adapter, spdf=res_leads, path=path_to_leads, mins_to_wait=60)
            safe_append_spark(yt_adapter=yt_adapter, spdf=res_leads, path=path_to_history, mins_to_wait=60)

    if new_leads_count > 0:
        yt_adapter.optimize_chunk_number(path_to_leads)
        yt_adapter.optimize_chunk_number(path_to_history)


if __name__ == '__main__':
    make_mkt_promo_leads()
