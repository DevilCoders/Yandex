import os
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"
import pip  # needed to use the pip functions
pip.main(['freeze'])

import yaml
import json
import click
import numpy as np
import pandas as pd
import logging.config
import spyt
from typing import List, Dict, Any
from spyt import spark_session
import pyspark.sql.functions as F
from pyspark.sql.functions import col, lit
from datetime import datetime, timedelta
from functools import partial

from clan_tools.utils.spark import SPARK_CONF_MEDIUM
from clan_tools.utils.spark import safe_append_spark
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from capacity_planning.utils.series import TimeSeriesForecast, SarimaxParams

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


FORECAST_SCHEMA_LIST = [
    'rep_date string',
    'crm_segment string',
    'billing_account_state string',
    'billing_account_usage_status string',
    'mean double',
    'mean_se double',
    'mean_ci_lower double',
    'mean_ci_upper double'
]


def linear_fill(df: pd.DataFrame, fix_periods: List, date_col: str, fix_col: str) -> pd.DataFrame:
    """Preprocess outliers according with config"""
    res_df = df.reset_index(drop=True).sort_values(date_col)
    for per in fix_periods:
        date_start = per['start']
        date_end = per['end']
        n_days = (datetime.strptime(date_end, '%Y-%m-%d') - datetime.strptime(date_start, '%Y-%m-%d')).days

        ts = res_df.loc[(res_df[date_col] >= date_start) & (res_df[date_col] <= date_end), fix_col].values
        if len(ts) == n_days + 1:
            lin_ts = (np.arange(n_days + 1) * (ts[-1] - ts[0]) / n_days + ts[0]).round(2)
            res_df.loc[(res_df[date_col] >= date_start) & (res_df[date_col] <= date_end), fix_col] = lin_ts

    return res_df


def predict_fip(df: pd.DataFrame, cfg: Dict[str, Any]):
    """Parse configs from `cfg` and make forecast for particular timeseries.
    Specific for IPv4 data.
    """
    state = df['billing_account_state'].iloc[0]
    status = df['billing_account_usage_status'].iloc[0]
    segment = df['crm_segment'].iloc[0]

    f_days = cfg[segment][state][status]['forecast_days']
    h_days = cfg[segment][state][status]['history_days']
    f_type = cfg[segment][state][status]['forecast_type']
    date_from = cfg[segment][state][status]['date_from']

    df = df[df['rep_date'] >= date_from].iloc[-h_days:]
    tsf = TimeSeriesForecast('ip_count', 'rep_date')
    endog, exog, exog_fut = tsf.prepare_sample(df, h_days=h_days, f_days=f_days)

    if f_type == 'const':
        res = tsf.make_const_prediction(endog, exog, exog_fut)
    elif f_type == 'simple':
        res = tsf.make_sarimax_prediction(endog, exog, exog_fut)
    elif f_type == 'advanced':
        params = cfg[segment][state][status]['params']
        colnames = cfg[segment][state][status]['colnames']

        res = tsf.make_sarimax_prediction(endog=endog, exog=exog, exog_fut=exog_fut, colnames=colnames, param=SarimaxParams.from_dict(params))
    else:
        res = tsf.make_nan_prediction(endog, exog, exog_fut)

    res['billing_account_state'] = state
    res['billing_account_usage_status'] = status
    res['crm_segment'] = segment
    res.index.name = 'rep_date'
    res = res.reset_index()
    res['rep_date'] = res['rep_date'].dt.strftime('%Y-%m-%d')
    res = res[['rep_date', 'crm_segment', 'billing_account_state', 'billing_account_usage_status',
               'mean', 'mean_se', 'mean_ci_lower', 'mean_ci_upper']]
    return res


@click.command()
@click.option('--is_prod', is_flag=True, default=False)
def predict_ts(is_prod: bool):
    """Makes forecast

    Args:
        is_prod (bool): if False workflow works with test tables and sources (used for debug)
    """

    logger.info('Starting predict Floating IPv4 capacity...')
    yt_token = os.environ["SPARK_SECRET"] if "SPARK_SECRET" in os.environ else os.environ["YT_TOKEN"]
    yt_adapter = YTAdapter(yt_token)

    project_path = '//home/cloud_analytics/ml/capacity_planning/fip4'
    outliers_dates = f'{project_path}/cfg/fix_dates.yaml'
    training_params = f'{project_path}/cfg/fip4_config.json'
    source_table = f'{project_path}/historical_data'
    result_table = f'{project_path}/forecast' + ('' if is_prod else '_test')

    result_table_name = result_table.split('/')[-1]
    result_table_folder = '/'.join(result_table.split('/')[:-1])

    source_table_name = source_table.split('/')[-1]
    source_table_folder = '/'.join(source_table.split('/')[:-1])

    fix_periods = yaml.safe_load(yt_adapter.yt.read_file(outliers_dates).read())['fix_periods']
    fip4_config = json.load(yt_adapter.yt.read_file(training_params))

    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_MEDIUM) as spark:
        spyt.info(spark)

        # define start and end dates to append in historical data table
        start_dt = '2021-12-01'
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
            spdf_schema = spark.read.yt(source_table).schema
            group_colnames = ['crm_segment', 'billing_account_state', 'billing_account_usage_status']
        else:
            raise RuntimeError(f'Can not resolve source path in {source_table}')

        logger.info(f'Make forecast in period: {start_dt} - {end_dt}')

        spdf_source = spark.read.yt(source_table).cache()
        dates = pd.date_range(start=start_dt, end=end_dt, freq='1D').strftime('%Y-%m-%d')
        for i, training_date in enumerate(dates):
            logger.debug(f'Handling date {i+1}/{len(dates)} - {training_date}')
            spdf = (
                spdf_source
                .filter(col('rep_date') < training_date).sort('rep_date')
                .filter(col('ip_count').isNotNull())
                .groupby(group_colnames)
                .applyInPandas(partial(linear_fill, fix_periods=fix_periods, date_col='rep_date', fix_col='ip_count'), spdf_schema)
                .groupby(group_colnames)
                .applyInPandas(partial(predict_fip, cfg=fip4_config), schema=', '.join(FORECAST_SCHEMA_LIST))
                .withColumn('training_date', lit(training_date))
                .coalesce(1)
            )
            safe_append_spark(yt_adapter=yt_adapter, spdf=spdf, path=result_table, mins_to_wait=15)

        # if is_prod and start_dt <= end_dt:
        #     logger.info('Make fast access table')
        #     fast_access_dt = (datetime.now() - timedelta(days=30)).strftime('%Y-%m-%d')
        #     fast_access_table = result_table + '_last_30d'
        #     fast_access_copy = (
        #         spark.read.yt(result_table)
        #         .filter(col('training_date') > fast_access_dt)
        #         .sort('training_date', 'platform', 'zone', 'rep_date')
        #         .coalesce(1)
        #     )
        #     fast_access_copy.write.optimize_for("scan").yt(fast_access_table, mode='overwrite')

    yt_adapter.optimize_chunk_number(result_table)

if __name__ == '__main__':
    predict_ts()
