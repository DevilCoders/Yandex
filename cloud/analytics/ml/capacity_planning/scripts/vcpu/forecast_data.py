# pylint: disable=no-value-for-parameter
import os
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"
import pip  # needed to use the pip functions
pip.main(['freeze'])

import click
import pandas as pd
import logging.config
import statsmodels.api as sm
import spyt
from spyt import spark_session
import pyspark.sql.functions as F
from pyspark.sql.functions import col, lit
from datetime import datetime, timedelta
from clan_tools.utils.spark import SPARK_CONF_MEDIUM
from clan_tools.utils.spark import safe_append_spark
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def forecast_metric(ts: pd.DataFrame) -> pd.DataFrame:
    platform = ts['platform'].unique()[0]
    zone = ts['zone'].unique()[0]
    ts['rep_date'] = pd.to_datetime(ts['rep_date'])
    ts = ts.set_index('rep_date').resample('1D').max().ffill()

    forecast_from = ts.index[-1] + timedelta(days=1)
    forecast_to = forecast_from + timedelta(days=365*2)
    history_from = ts.index[-1] - timedelta(days=365*2)

    def make_forecast(pds: pd.Series) -> pd.DataFrame:
        name = pds.name
        try:
            assert len(pds) > 30
            res = (
                sm.tsa.SARIMAX(pds, order=(1, 1, 1), trend='t')
                .fit(maxiter=200)
                .get_forecast(forecast_to)
                .summary_frame()
                .round(0)
            )
            res.columns = res.columns.str.replace('mean', name)
        except Exception:
            index = pd.date_range(start=forecast_from, end=forecast_to)
            columns = [name, name+'_se', name+'_ci_lower', name+'_ci_upper']
            res = pd.DataFrame(index=index, columns=columns, dtype=float)
        return res

    res_used = make_forecast(ts['used'][ts.index>history_from])
    res_used_wop = make_forecast(ts['used_wop'][ts.index>history_from])
    res_df = res_used.join(res_used_wop, how='outer').reset_index()
    res_df.columns = ['rep_date'] + res_df.columns.tolist()[1:]
    res_df['rep_date'] = res_df['rep_date'].dt.strftime('%Y-%m-%d')
    final_df = res_df.copy()
    final_df['platform'] = platform
    final_df['zone'] = zone
    final_df = final_df[['platform', 'zone'] + res_df.columns.tolist()]

    return final_df


@click.command()
@click.option('--env')
@click.option('--is_prod', is_flag=True, default=False)
def predict_ts(env: str, is_prod: bool):
    """Makes forecast

    Args:
        env (str): prod/preprod environment of resources (must be `prod` or `preprod`)
        is_prod (bool): if False workflow works with test tables and sources (used for debug)
    """

    logger.info(f'Starting predict {env} vCPU capacity...')
    yt_token = os.environ["SPARK_SECRET"] if "SPARK_SECRET" in os.environ else os.environ["YT_TOKEN"]
    yt_adapter = YTAdapter(yt_token)

    prod_postfix = '' if is_prod else '_test'
    source_table = f'//home/cloud_analytics/ml/capacity_planning/vcpu/historical_data_{env}'
    source_table_name = source_table.split('/')[-1]
    source_table_folder = '/'.join(source_table.split('/')[:-1])
    result_table = f'//home/cloud_analytics/ml/capacity_planning/vcpu/forecast_{env}{prod_postfix}'
    result_table_name = result_table.split('/')[-1]
    result_table_folder = '/'.join(result_table.split('/')[:-1])

    # spark session
    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_MEDIUM) as spark:
        spyt.info(spark)

        # define start and end dates to append in historical data table
        start_dt = '2021-06-01'
        if result_table_name in yt_adapter.yt.list(result_table_folder):
            if is_prod:
                last_forecast_dt = spark.read.yt(result_table).agg(F.max('training_date')).collect()[0][0]
                start_dt = (datetime.strptime(last_forecast_dt, '%Y-%m-%d') + timedelta(days=1)).strftime('%Y-%m-%d')
            else:
                logger.info(f'Delete {result_table}')
                yt_adapter.yt.remove(result_table)
        if not is_prod:
            start_dt = (datetime.now() - timedelta(days=5)).strftime('%Y-%m-%d')

        if source_table_name in yt_adapter.yt.list(source_table_folder):
            today = datetime.now().strftime('%Y-%m-%d')
            last_source_dt = spark.read.yt(source_table).agg(F.max('rep_date')).collect()[0][0]
            last_date_for_training = (datetime.strptime(last_source_dt, '%Y-%m-%d') + timedelta(days=1)).strftime('%Y-%m-%d')
            end_dt = min(today, last_date_for_training)
        else:
            raise RuntimeError(f'Can not resolve source path in {source_table}')
        logger.info(f'Make forecast in period: {start_dt} - {end_dt}')

        spdf = spark.read.yt(source_table).cache()
        schema_list = ['platform string',
                       'zone string',
                       'rep_date string',
                       'used double',
                       'used_se double',
                       'used_ci_lower double',
                       'used_ci_upper double',
                       'used_wop double',
                       'used_wop_se double',
                       'used_wop_ci_lower double',
                       'used_wop_ci_upper double']

        dates = pd.date_range(start=start_dt, end=end_dt, freq='1D').strftime('%Y-%m-%d')
        for i, date in enumerate(dates):
            logger.debug(f'Handling date {i+1}/{len(dates)} - {date}')
            results = (
                spdf
                .filter(col('rep_date') < date)
                .groupBy('platform', 'zone')
                .applyInPandas(forecast_metric, schema=', '.join(schema_list))
                .withColumn('training_date', lit(date))
                .coalesce(1)
            )
            safe_append_spark(yt_adapter=yt_adapter, spdf=results, path=result_table, mins_to_wait=60)

        if is_prod and start_dt <= end_dt:
            logger.info('Make fast access table')
            fast_access_dt = (datetime.now() - timedelta(days=30)).strftime('%Y-%m-%d')
            fast_access_table = result_table + '_last_30d'
            fast_access_copy = (
                spark.read.yt(result_table)
                .filter(col('training_date') > fast_access_dt)
                .sort('training_date', 'platform', 'zone', 'rep_date')
                .coalesce(1)
            )
            fast_access_copy.write.optimize_for("scan").yt(fast_access_table, mode='overwrite')

    yt_adapter.optimize_chunk_number(result_table)

if __name__ == '__main__':
    predict_ts()
